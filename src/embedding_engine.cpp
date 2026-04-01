#include "embedding_engine.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <optional>
#include <vector>

#ifdef USE_ONNXRUNTIME
#include <onnxruntime_cxx_api.h>
#endif

EmbeddingEngine::EmbeddingEngine() = default;

EmbeddingEngine::~EmbeddingEngine() = default;

EmbeddingEngine::EmbeddingEngine(EmbeddingEngine&&) noexcept = default;
EmbeddingEngine&
EmbeddingEngine::operator=(EmbeddingEngine&&) noexcept = default;

// Content-based embeddings are always available regardless of ONNX.
bool EmbeddingEngine::is_enabled() const noexcept { return true; }

bool EmbeddingEngine::initialize(const fs::path& model_path) {
#ifdef USE_ONNXRUNTIME
   if (initialized_) {
      return true;
   }

   if (!fs::exists(model_path)) {
      return false;
   }

   try {
      env_ = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING,
                                        "EmbeddingEngine");

      session_options_ = std::make_unique<Ort::SessionOptions>();
      session_options_->SetIntraOpNumThreads(1);
      session_options_->SetGraphOptimizationLevel(
          GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

      session_ = std::make_unique<Ort::Session>(*env_, model_path.c_str(),
                                                *session_options_);

      initialized_ = true;
      return true;
   } catch (const Ort::Exception&) {
      return false;
   }
#else
   (void)model_path;
   return false;
#endif
}

std::optional<std::vector<float>>
EmbeddingEngine::embed_file(const fs::path& file_path) {
#ifdef USE_ONNXRUNTIME
   if (initialized_) {
      return embed_image(file_path);
   }
#endif
   return embed_file_content(file_path);
}

std::optional<std::vector<float>>
EmbeddingEngine::embed_file_content(const fs::path& file_path) {
   if (!fs::exists(file_path) || !fs::is_regular_file(file_path)) {
      return std::nullopt;
   }

   std::ifstream file(file_path, std::ios::binary);
   if (!file)
      return std::nullopt;

   // Accumulate per-dimension SimHash votes over 64-byte blocks.
   // Each of the 128 dimensions uses FNV-1a with a different seed so that
   // hash bits are (pseudo-)independent across dimensions.
   constexpr size_t DIM = CONTENT_EMBEDDING_DIM;
   constexpr size_t BLOCK_SIZE = 64;
   constexpr uint32_t FNV_PRIME = 16777619u;
   constexpr uint32_t FNV_OFFSET = 2166136261u;
   constexpr uint32_t SEED_MULT = 2654435761u; // Knuth multiplicative constant

   std::array<int32_t, DIM> votes{};
   votes.fill(0);

   std::array<uint8_t, BLOCK_SIZE> block{};
   size_t total_bytes = 0;

   while (true) {
      file.read(reinterpret_cast<char*>(block.data()),
                static_cast<std::streamsize>(BLOCK_SIZE));
      const size_t bytes_read = static_cast<size_t>(file.gcount());
      if (bytes_read == 0)
         break;
      total_bytes += bytes_read;

      for (size_t dim = 0; dim < DIM; ++dim) {
         // Mix per-dimension seed into the FNV offset.
         uint32_t h = FNV_OFFSET ^ static_cast<uint32_t>(dim * SEED_MULT);
         for (size_t b = 0; b < bytes_read; ++b) {
            h ^= block[b];
            h *= FNV_PRIME;
         }
         // High bit determines vote direction for this dimension.
         votes[dim] += (h & 0x80000000u) ? 1 : -1;
      }

      block.fill(0);
   }

   if (total_bytes == 0) {
      return std::nullopt; // Empty file — no meaningful embedding.
   }

   // Convert accumulated votes to floats and L2-normalize.
   std::vector<float> embedding(DIM);
   float norm_sq = 0.0f;
   for (size_t i = 0; i < DIM; ++i) {
      embedding[i] = static_cast<float>(votes[i]);
      norm_sq += embedding[i] * embedding[i];
   }

   const float norm = std::sqrt(norm_sq);
   if (norm > 0.0f) {
      for (float& v : embedding)
         v /= norm;
   }

   return embedding;
}

std::optional<std::vector<float>>
EmbeddingEngine::embed_image(const fs::path& image_path) {
#ifdef USE_ONNXRUNTIME
   if (!initialized_ || !fs::exists(image_path)) {
      return std::nullopt;
   }

   try {
      Ort::AllocatorWithDefaultOptions allocator;

      const size_t num_input_nodes = session_->GetInputCount();
      const size_t num_output_nodes = session_->GetOutputCount();

      if (num_input_nodes == 0 || num_output_nodes == 0) {
         return std::nullopt;
      }

      auto input_name_ptr = session_->GetInputNameAllocated(0, allocator);
      auto output_name_ptr = session_->GetOutputNameAllocated(0, allocator);

      const char* input_name = input_name_ptr.get();
      const char* output_name = output_name_ptr.get();

      const std::array<int64_t, 4> input_shape{1, 3, 224, 224};
      const size_t input_tensor_size = 1 * 3 * 224 * 224;
      std::vector<float> input_tensor_values(input_tensor_size, 0.5f);

      auto memory_info =
          Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
      Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
          memory_info, input_tensor_values.data(), input_tensor_size,
          input_shape.data(), input_shape.size());

      std::vector<const char*> input_names{input_name};
      std::vector<const char*> output_names{output_name};

      auto output_tensors =
          session_->Run(Ort::RunOptions{nullptr}, input_names.data(),
                        &input_tensor, 1, output_names.data(), 1);

      if (output_tensors.empty()) {
         return std::nullopt;
      }

      float* output_data = output_tensors[0].GetTensorMutableData<float>();
      auto type_info = output_tensors[0].GetTensorTypeAndShapeInfo();
      size_t output_size = type_info.GetElementCount();

      std::vector<float> embedding(output_data, output_data + output_size);

      float norm = 0.0f;
      for (float val : embedding) {
         norm += val * val;
      }
      norm = std::sqrt(norm);

      if (norm > 0.0f) {
         for (float& val : embedding) {
            val /= norm;
         }
      }

      return embedding;

   } catch (const Ort::Exception&) {
      return std::nullopt;
   }
#else
   (void)image_path;
   return std::nullopt;
#endif
}
