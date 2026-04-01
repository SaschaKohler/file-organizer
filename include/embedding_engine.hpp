#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

namespace fs = std::filesystem;

#ifdef USE_ONNXRUNTIME
namespace Ort {
struct Env;
struct Session;
struct SessionOptions;
} // namespace Ort
#endif

class EmbeddingEngine {
 public:
   // Embedding dimensionality used by the content-based (non-ONNX) method.
   static constexpr size_t CONTENT_EMBEDDING_DIM = 128;

   EmbeddingEngine();
   ~EmbeddingEngine();

   EmbeddingEngine(const EmbeddingEngine&) = delete;
   EmbeddingEngine& operator=(const EmbeddingEngine&) = delete;
   EmbeddingEngine(EmbeddingEngine&&) noexcept;
   EmbeddingEngine& operator=(EmbeddingEngine&&) noexcept;

   // Always returns true: content-based embeddings work without ONNX.
   // When built with USE_ONNXRUNTIME and a model is loaded, ONNX inference
   // is used instead.
   bool is_enabled() const noexcept;

   // Load an ONNX model. Required only when USE_ONNXRUNTIME is defined.
   // Returns false if ONNX is not compiled in or model path is invalid.
   bool initialize(const fs::path& model_path);

   // Embed a file using ONNX (if initialized) or content-based SimHash.
   // Returns std::nullopt for non-existent or empty files.
   std::optional<std::vector<float>> embed_file(const fs::path& file_path);

   // ONNX-specific image embedding (requires USE_ONNXRUNTIME + initialize()).
   std::optional<std::vector<float>> embed_image(const fs::path& image_path);

   // Content-based SimHash embedding — works without any ML model.
   // Divides file into 64-byte blocks and accumulates per-dimension votes
   // (FNV-1a hashes with 128 seeds). Result is L2-normalized.
   // Returns std::nullopt for non-existent or empty files.
   std::optional<std::vector<float>>
   embed_file_content(const fs::path& file_path);

 private:
#ifdef USE_ONNXRUNTIME
   std::unique_ptr<Ort::Env> env_;
   std::unique_ptr<Ort::Session> session_;
   std::unique_ptr<Ort::SessionOptions> session_options_;
   bool initialized_{false};
#endif
};
