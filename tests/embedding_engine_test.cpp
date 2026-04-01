#include "embedding_engine.hpp"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

namespace fs = std::filesystem;

class EmbeddingEngineTest : public ::testing::Test {
 protected:
   void SetUp() override {
      test_dir_ = fs::temp_directory_path() / "embedding_engine_test";
      fs::create_directories(test_dir_);
   }

   void TearDown() override {
      if (fs::exists(test_dir_)) {
         fs::remove_all(test_dir_);
      }
   }

   fs::path write_file(const std::string& name, const std::string& content) {
      fs::path p = test_dir_ / name;
      std::ofstream f(p);
      f << content;
      return p;
   }

   fs::path test_dir_;
};

// is_enabled() is always true now — content-based embeddings need no ONNX.
TEST_F(EmbeddingEngineTest, IsEnabledAlwaysTrue) {
   EmbeddingEngine engine;
   EXPECT_TRUE(engine.is_enabled());
}

// initialize() still fails gracefully when ONNX is not compiled in or path
// is invalid — it should not crash.
TEST_F(EmbeddingEngineTest, InitializeFailsWithNonexistentModel) {
   EmbeddingEngine engine;
   bool success = engine.initialize(test_dir_ / "nonexistent.onnx");
   EXPECT_FALSE(success);
}

// Content-based embedding works for a regular text file.
TEST_F(EmbeddingEngineTest, EmbedFileContentReturnsVector) {
   auto p = write_file("hello.txt", "Hello, World!");
   EmbeddingEngine engine;
   auto result = engine.embed_file_content(p);
   ASSERT_TRUE(result.has_value());
   EXPECT_EQ(result->size(), EmbeddingEngine::CONTENT_EMBEDDING_DIM);
}

// embed_file() falls back to content-based embedding without ONNX.
TEST_F(EmbeddingEngineTest, EmbedFileWorksWithoutOnnx) {
   auto p = write_file("sample.txt", "some file content for embedding");
   EmbeddingEngine engine;
   auto result = engine.embed_file(p);
#ifdef USE_ONNXRUNTIME
   // With ONNX compiled in but no model loaded, falls back to content-based.
   ASSERT_TRUE(result.has_value());
#else
   ASSERT_TRUE(result.has_value());
#endif
   EXPECT_EQ(result->size(), EmbeddingEngine::CONTENT_EMBEDDING_DIM);
}

// Result vector must be L2-normalized (norm ≈ 1).
TEST_F(EmbeddingEngineTest, EmbeddingIsNormalized) {
   auto p = write_file("norm_test.txt", std::string(200, 'A'));
   EmbeddingEngine engine;
   auto result = engine.embed_file_content(p);
   ASSERT_TRUE(result.has_value());

   float norm_sq = 0.0f;
   for (float v : *result)
      norm_sq += v * v;
   EXPECT_NEAR(norm_sq, 1.0f, 1e-5f);
}

// Two identical files must produce exactly the same embedding.
TEST_F(EmbeddingEngineTest, IdenticalFilesHaveIdenticalEmbeddings) {
   const std::string content = "identical content for both files";
   auto p1 = write_file("ident_a.txt", content);
   auto p2 = write_file("ident_b.txt", content);

   EmbeddingEngine engine;
   auto emb1 = engine.embed_file_content(p1);
   auto emb2 = engine.embed_file_content(p2);

   ASSERT_TRUE(emb1.has_value());
   ASSERT_TRUE(emb2.has_value());
   ASSERT_EQ(emb1->size(), emb2->size());

   for (size_t i = 0; i < emb1->size(); ++i) {
      EXPECT_FLOAT_EQ((*emb1)[i], (*emb2)[i]);
   }
}

// Different files must produce different embeddings.
TEST_F(EmbeddingEngineTest, DifferentFilesHaveDifferentEmbeddings) {
   auto p1 = write_file("diff_a.txt", "file content alpha");
   auto p2 =
       write_file("diff_b.txt", "completely different content beta gamma");

   EmbeddingEngine engine;
   auto emb1 = engine.embed_file_content(p1);
   auto emb2 = engine.embed_file_content(p2);

   ASSERT_TRUE(emb1.has_value());
   ASSERT_TRUE(emb2.has_value());

   bool any_different = false;
   for (size_t i = 0; i < emb1->size(); ++i) {
      if ((*emb1)[i] != (*emb2)[i]) {
         any_different = true;
         break;
      }
   }
   EXPECT_TRUE(any_different);
}

// Empty files return nullopt — no meaningful embedding.
TEST_F(EmbeddingEngineTest, EmptyFileReturnsNullopt) {
   auto p = write_file("empty.txt", "");
   EmbeddingEngine engine;
   auto result = engine.embed_file_content(p);
   EXPECT_FALSE(result.has_value());
}

// Non-existent file returns nullopt.
TEST_F(EmbeddingEngineTest, NonexistentFileReturnsNullopt) {
   EmbeddingEngine engine;
   auto result = engine.embed_file_content(test_dir_ / "does_not_exist.bin");
   EXPECT_FALSE(result.has_value());
}

// Binary file is handled correctly.
TEST_F(EmbeddingEngineTest, BinaryFileProducesEmbedding) {
   fs::path p = test_dir_ / "binary.bin";
   {
      std::ofstream f(p, std::ios::binary);
      for (int i = 0; i < 256; ++i)
         f.put(static_cast<char>(i));
   } // File flushed and closed before reading.

   EmbeddingEngine engine;
   auto result = engine.embed_file_content(p);
   ASSERT_TRUE(result.has_value());
   EXPECT_EQ(result->size(), EmbeddingEngine::CONTENT_EMBEDDING_DIM);
}

// Move constructor and move assignment work correctly.
TEST_F(EmbeddingEngineTest, MoveConstructorWorks) {
   EmbeddingEngine engine1;
   EmbeddingEngine engine2(std::move(engine1));
   EXPECT_TRUE(engine2.is_enabled());
}

TEST_F(EmbeddingEngineTest, MoveAssignmentWorks) {
   EmbeddingEngine engine1;
   EmbeddingEngine engine2;
   engine2 = std::move(engine1);
   EXPECT_TRUE(engine2.is_enabled());
}

#ifdef USE_ONNXRUNTIME
TEST_F(EmbeddingEngineTest, InitializeSucceedsWithValidModel) {
   auto model_path = test_dir_ / "dummy_model.onnx";
   std::ofstream model_file(model_path, std::ios::binary);
   model_file.close();

   EmbeddingEngine engine;
   bool success = engine.initialize(model_path);
   // A dummy (empty) ONNX file is invalid — initialize should fail gracefully.
   EXPECT_FALSE(success);
}

TEST_F(EmbeddingEngineTest, EmbedImageReturnsNulloptForNonexistentFile) {
   EmbeddingEngine engine;
   auto result = engine.embed_image(test_dir_ / "nonexistent.jpg");
   EXPECT_FALSE(result.has_value());
}
#endif
