#include "duplicate_detector.hpp"
#include "embedding_engine.hpp"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

namespace fs = std::filesystem;

class DuplicateDetectorTest : public ::testing::Test {
 protected:
   fs::path test_dir_;
   std::unique_ptr<EmbeddingEngine> engine_;
   std::unique_ptr<DuplicateDetector> detector_;

   void SetUp() override {
      test_dir_ = fs::temp_directory_path() / "duplicate_detector_test";
      fs::create_directories(test_dir_);

      engine_ = std::make_unique<EmbeddingEngine>();
      detector_ = std::make_unique<DuplicateDetector>(*engine_, 0.95f);
   }

   void TearDown() override {
      if (fs::exists(test_dir_)) {
         fs::remove_all(test_dir_);
      }
   }

   void create_file(const std::string& filename, const std::string& content) {
      std::ofstream file(test_dir_ / filename);
      file << content;
   }

   FileInfo create_file_info(const std::string& filename) {
      return FileInfo(test_dir_ / filename);
   }
};

TEST_F(DuplicateDetectorTest, NoFilesReturnsEmpty) {
   std::vector<FileInfo> files;
   auto duplicates = detector_->find_exact_duplicates(files);
   EXPECT_TRUE(duplicates.empty());
}

TEST_F(DuplicateDetectorTest, SingleFileReturnsEmpty) {
   create_file("file1.txt", "content");
   std::vector<FileInfo> files = {create_file_info("file1.txt")};

   auto duplicates = detector_->find_exact_duplicates(files);
   EXPECT_TRUE(duplicates.empty());
}

TEST_F(DuplicateDetectorTest, TwoIdenticalFilesDetected) {
   create_file("file1.txt", "identical content");
   create_file("file2.txt", "identical content");

   std::vector<FileInfo> files = {create_file_info("file1.txt"),
                                  create_file_info("file2.txt")};

   auto duplicates = detector_->find_exact_duplicates(files);

   ASSERT_EQ(duplicates.size(), 1);
   EXPECT_EQ(duplicates[0].files.size(), 2);
   EXPECT_FLOAT_EQ(duplicates[0].similarity_score, 1.0f);
}

TEST_F(DuplicateDetectorTest, ThreeIdenticalFilesDetected) {
   create_file("file1.txt", "same content");
   create_file("file2.txt", "same content");
   create_file("file3.txt", "same content");

   std::vector<FileInfo> files = {create_file_info("file1.txt"),
                                  create_file_info("file2.txt"),
                                  create_file_info("file3.txt")};

   auto duplicates = detector_->find_exact_duplicates(files);

   ASSERT_EQ(duplicates.size(), 1);
   EXPECT_EQ(duplicates[0].files.size(), 3);
   EXPECT_FLOAT_EQ(duplicates[0].similarity_score, 1.0f);
}

TEST_F(DuplicateDetectorTest, DifferentFilesNotDetected) {
   create_file("file1.txt", "content A");
   create_file("file2.txt", "content B");
   create_file("file3.txt", "content C");

   std::vector<FileInfo> files = {create_file_info("file1.txt"),
                                  create_file_info("file2.txt"),
                                  create_file_info("file3.txt")};

   auto duplicates = detector_->find_exact_duplicates(files);
   EXPECT_TRUE(duplicates.empty());
}

TEST_F(DuplicateDetectorTest, MultipleDuplicateGroups) {
   create_file("file1.txt", "group A");
   create_file("file2.txt", "group A");
   create_file("file3.txt", "group B");
   create_file("file4.txt", "group B");
   create_file("file5.txt", "unique");

   std::vector<FileInfo> files = {
       create_file_info("file1.txt"), create_file_info("file2.txt"),
       create_file_info("file3.txt"), create_file_info("file4.txt"),
       create_file_info("file5.txt")};

   auto duplicates = detector_->find_exact_duplicates(files);

   ASSERT_EQ(duplicates.size(), 2);
   for (const auto& group : duplicates) {
      EXPECT_EQ(group.files.size(), 2);
      EXPECT_FLOAT_EQ(group.similarity_score, 1.0f);
   }
}

TEST_F(DuplicateDetectorTest, EmptyFilesAreDetected) {
   create_file("empty1.txt", "");
   create_file("empty2.txt", "");

   std::vector<FileInfo> files = {create_file_info("empty1.txt"),
                                  create_file_info("empty2.txt")};

   auto duplicates = detector_->find_exact_duplicates(files);

   ASSERT_EQ(duplicates.size(), 1);
   EXPECT_EQ(duplicates[0].files.size(), 2);
}

TEST_F(DuplicateDetectorTest, LargeFilesDetected) {
   std::string large_content(10000, 'X');
   create_file("large1.txt", large_content);
   create_file("large2.txt", large_content);

   std::vector<FileInfo> files = {create_file_info("large1.txt"),
                                  create_file_info("large2.txt")};

   auto duplicates = detector_->find_exact_duplicates(files);

   ASSERT_EQ(duplicates.size(), 1);
   EXPECT_EQ(duplicates[0].files.size(), 2);
}

TEST_F(DuplicateDetectorTest, BinaryFilesDetected) {
   std::ofstream file1(test_dir_ / "binary1.bin", std::ios::binary);
   std::ofstream file2(test_dir_ / "binary2.bin", std::ios::binary);

   unsigned char data[] = {0x00, 0xFF, 0xAA, 0x55, 0x12, 0x34};
   file1.write(reinterpret_cast<char*>(data), sizeof(data));
   file2.write(reinterpret_cast<char*>(data), sizeof(data));
   file1.close();
   file2.close();

   std::vector<FileInfo> files = {create_file_info("binary1.bin"),
                                  create_file_info("binary2.bin")};

   auto duplicates = detector_->find_exact_duplicates(files);

   ASSERT_EQ(duplicates.size(), 1);
   EXPECT_EQ(duplicates[0].files.size(), 2);
}

TEST_F(DuplicateDetectorTest, SimilarButNotIdenticalNotDetected) {
   create_file("file1.txt", "content A");
   create_file("file2.txt", "content B");

   std::vector<FileInfo> files = {create_file_info("file1.txt"),
                                  create_file_info("file2.txt")};

   auto duplicates = detector_->find_exact_duplicates(files);
   EXPECT_TRUE(duplicates.empty());
}

TEST_F(DuplicateDetectorTest, DifferentSizesNotDetected) {
   create_file("file1.txt", "short");
   create_file("file2.txt", "much longer content");

   std::vector<FileInfo> files = {create_file_info("file1.txt"),
                                  create_file_info("file2.txt")};

   auto duplicates = detector_->find_exact_duplicates(files);
   EXPECT_TRUE(duplicates.empty());
}

TEST_F(DuplicateDetectorTest, SetAndGetThreshold) {
   EXPECT_FLOAT_EQ(detector_->get_similarity_threshold(), 0.95f);

   detector_->set_similarity_threshold(0.85f);
   EXPECT_FLOAT_EQ(detector_->get_similarity_threshold(), 0.85f);

   detector_->set_similarity_threshold(0.99f);
   EXPECT_FLOAT_EQ(detector_->get_similarity_threshold(), 0.99f);
}

TEST_F(DuplicateDetectorTest, FindDuplicatesDetectsIdenticalFilesViaEmbedding) {
   // EmbeddingEngine is always enabled now (content-based SimHash).
   // Identical files must produce identical embeddings → found as duplicates.
   create_file("file1.txt", "identical content for embedding test");
   create_file("file2.txt", "identical content for embedding test");

   std::vector<FileInfo> files = {create_file_info("file1.txt"),
                                  create_file_info("file2.txt")};

   ASSERT_TRUE(engine_->is_enabled());
   auto duplicates = detector_->find_duplicates(files);

   ASSERT_EQ(duplicates.size(), 1u);
   EXPECT_EQ(duplicates[0].files.size(), 2u);
}

TEST_F(DuplicateDetectorTest, FindDuplicatesDetectsNearDuplicates) {
   // Build two large files that differ only in one 64-byte block.
   // SimHash embeddings should be highly similar → detected as duplicates.
   const std::string base(640, 'X'); // 10 × 64-byte blocks of 'X'
   std::string modified = base;
   // Change a few bytes in the first block only.
   modified[0] = 'A';
   modified[1] = 'B';
   modified[2] = 'C';

   create_file("near_dup_a.txt", base);
   create_file("near_dup_b.txt", modified);

   std::vector<FileInfo> files = {create_file_info("near_dup_a.txt"),
                                  create_file_info("near_dup_b.txt")};

   // Use a lower threshold to catch near-duplicates.
   detector_->set_similarity_threshold(0.85f);
   auto duplicates = detector_->find_duplicates(files);

   // With 9/10 blocks identical the embeddings should be very similar.
   EXPECT_GE(duplicates.size(), 1u);
}

TEST_F(DuplicateDetectorTest, WhitespaceOnlyFilesDetected) {
   create_file("space1.txt", "   ");
   create_file("space2.txt", "   ");

   std::vector<FileInfo> files = {create_file_info("space1.txt"),
                                  create_file_info("space2.txt")};

   auto duplicates = detector_->find_exact_duplicates(files);

   ASSERT_EQ(duplicates.size(), 1);
   EXPECT_EQ(duplicates[0].files.size(), 2);
}

TEST_F(DuplicateDetectorTest, NewlineVariationsNotDetected) {
   create_file("unix.txt", "line1\nline2\n");
   create_file("windows.txt", "line1\r\nline2\r\n");

   std::vector<FileInfo> files = {create_file_info("unix.txt"),
                                  create_file_info("windows.txt")};

   auto duplicates = detector_->find_exact_duplicates(files);
   EXPECT_TRUE(duplicates.empty());
}

TEST_F(DuplicateDetectorTest, UnicodeFilesDetected) {
   create_file("unicode1.txt", "Hello 世界 🌍");
   create_file("unicode2.txt", "Hello 世界 🌍");

   std::vector<FileInfo> files = {create_file_info("unicode1.txt"),
                                  create_file_info("unicode2.txt")};

   auto duplicates = detector_->find_exact_duplicates(files);

   ASSERT_EQ(duplicates.size(), 1);
   EXPECT_EQ(duplicates[0].files.size(), 2);
}

TEST_F(DuplicateDetectorTest, MixedDuplicatesAndUnique) {
   create_file("dup1.txt", "duplicate");
   create_file("dup2.txt", "duplicate");
   create_file("dup3.txt", "duplicate");
   create_file("unique1.txt", "unique A");
   create_file("unique2.txt", "unique B");

   std::vector<FileInfo> files = {
       create_file_info("dup1.txt"), create_file_info("unique1.txt"),
       create_file_info("dup2.txt"), create_file_info("unique2.txt"),
       create_file_info("dup3.txt")};

   auto duplicates = detector_->find_exact_duplicates(files);

   ASSERT_EQ(duplicates.size(), 1);
   EXPECT_EQ(duplicates[0].files.size(), 3);
}

TEST_F(DuplicateDetectorTest, PerformanceWithManyFiles) {
   for (int i = 0; i < 100; ++i) {
      create_file("file" + std::to_string(i) + ".txt",
                  "content " + std::to_string(i % 10));
   }

   std::vector<FileInfo> files;
   for (int i = 0; i < 100; ++i) {
      files.push_back(create_file_info("file" + std::to_string(i) + ".txt"));
   }

   auto duplicates = detector_->find_exact_duplicates(files);

   EXPECT_EQ(duplicates.size(), 10);
   for (const auto& group : duplicates) {
      EXPECT_EQ(group.files.size(), 10);
   }
}
