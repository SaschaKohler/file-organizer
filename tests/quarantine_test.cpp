#include "quarantine.hpp"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

namespace fs = std::filesystem;

class QuarantineTest : public ::testing::Test {
 protected:
   void SetUp() override {
      auto pid = std::to_string(getpid());
      base_dir_ = fs::temp_directory_path() / ("quarantine_test_q_" + pid);
      src_dir_ = fs::temp_directory_path() / ("quarantine_test_src_" + pid);
      fs::remove_all(base_dir_);
      fs::remove_all(src_dir_);
      fs::create_directories(base_dir_);
      fs::create_directories(src_dir_);
   }

   void TearDown() override {
      if (fs::exists(base_dir_))
         fs::remove_all(base_dir_);
      if (fs::exists(src_dir_))
         fs::remove_all(src_dir_);
   }

   // Write a file with given content, return its path.
   fs::path write_file(const std::string& name,
                       const std::string& content = "data") {
      fs::path p = src_dir_ / name;
      std::ofstream f(p);
      f << content;
      return p;
   }

   fs::path base_dir_;
   fs::path src_dir_;
};

// ─── quarantine_file
// ──────────────────────────────────────────────────────────

TEST_F(QuarantineTest, QuarantineMovesFileOutOfSource) {
   auto dup = write_file("dup.txt", "same");
   auto kept = write_file("orig.txt", "same");
   Quarantine q(base_dir_);

   auto result = q.quarantine_file(dup, kept, 1.0f);

   ASSERT_TRUE(result.has_value());
   EXPECT_FALSE(fs::exists(dup));    // removed from source
   EXPECT_TRUE(fs::exists(*result)); // present in quarantine
}

TEST_F(QuarantineTest, QuarantineCreatesBaseDir) {
   fs::path new_base = base_dir_ / "sub" / "dir";
   auto dup = write_file("dup2.txt");
   auto kept = write_file("orig2.txt");
   Quarantine q(new_base);

   auto result = q.quarantine_file(dup, kept, 1.0f);

   ASSERT_TRUE(result.has_value());
   EXPECT_TRUE(fs::exists(new_base));
}

TEST_F(QuarantineTest, QuarantineWritesManifest) {
   auto dup = write_file("dup3.txt", "content");
   auto kept = write_file("orig3.txt", "content");
   Quarantine q(base_dir_);

   q.quarantine_file(dup, kept, 1.0f, "hash");

   auto entries = q.load_manifest();
   ASSERT_EQ(entries.size(), 1u);
   EXPECT_EQ(entries[0].method, "hash");
   EXPECT_FLOAT_EQ(entries[0].similarity, 1.0f);
   EXPECT_EQ(entries[0].kept_path, kept);
}

TEST_F(QuarantineTest, QuarantineNonExistentFileReturnsNullopt) {
   Quarantine q(base_dir_);
   auto result = q.quarantine_file(src_dir_ / "no_such_file.txt",
                                   src_dir_ / "orig.txt", 1.0f);
   EXPECT_FALSE(result.has_value());
}

TEST_F(QuarantineTest, MultipleFilesGetUniqueQuarantinePaths) {
   auto f1 = write_file("file.txt", "alpha");
   // Small sleep so timestamps differ — unnecessary if unique names guaranteed.
   auto f2 = write_file("file2.txt", "beta");
   auto kept = write_file("orig.txt");
   Quarantine q(base_dir_);

   auto r1 = q.quarantine_file(f1, kept, 1.0f);
   auto r2 = q.quarantine_file(f2, kept, 1.0f);

   ASSERT_TRUE(r1.has_value());
   ASSERT_TRUE(r2.has_value());
   EXPECT_NE(*r1, *r2);
}

TEST_F(QuarantineTest, ManifestAccumulatesAcrossMultipleCalls) {
   auto kept = write_file("orig.txt");
   Quarantine q(base_dir_);

   for (int i = 0; i < 3; ++i) {
      auto f = write_file("dup" + std::to_string(i) + ".txt",
                          "content " + std::to_string(i));
      q.quarantine_file(f, kept, 1.0f);
   }

   auto entries = q.load_manifest();
   EXPECT_EQ(entries.size(), 3u);
}

// ─── undo_last
// ────────────────────────────────────────────────────────────────

TEST_F(QuarantineTest, UndoRestoresFile) {
   auto dup = write_file("undo_dup.txt", "original content");
   auto kept = write_file("undo_orig.txt", "original content");
   Quarantine q(base_dir_);

   auto qpath = q.quarantine_file(dup, kept, 1.0f);
   ASSERT_TRUE(qpath.has_value());
   ASSERT_FALSE(fs::exists(dup));

   bool ok = q.undo_last();

   EXPECT_TRUE(ok);
   EXPECT_TRUE(fs::exists(dup));     // restored
   EXPECT_FALSE(fs::exists(*qpath)); // removed from quarantine
}

TEST_F(QuarantineTest, UndoReturnsFalseOnEmptyStack) {
   Quarantine q(base_dir_);
   EXPECT_FALSE(q.undo_last());
}

TEST_F(QuarantineTest, UndoStackSizeTracksOperations) {
   auto kept = write_file("kept.txt");
   Quarantine q(base_dir_);
   EXPECT_EQ(q.undo_stack_size(), 0u);

   auto f1 = write_file("u1.txt");
   auto f2 = write_file("u2.txt");
   q.quarantine_file(f1, kept, 1.0f);
   EXPECT_EQ(q.undo_stack_size(), 1u);
   q.quarantine_file(f2, kept, 1.0f);
   EXPECT_EQ(q.undo_stack_size(), 2u);

   q.undo_last();
   EXPECT_EQ(q.undo_stack_size(), 1u);
   q.undo_last();
   EXPECT_EQ(q.undo_stack_size(), 0u);
}

TEST_F(QuarantineTest, UndoIsLifo) {
   auto kept = write_file("kept.txt");
   auto f1 = write_file("lifo1.txt", "first");
   auto f2 = write_file("lifo2.txt", "second");
   Quarantine q(base_dir_);

   q.quarantine_file(f1, kept, 1.0f);
   q.quarantine_file(f2, kept, 1.0f);

   // Undo should restore f2 first (LIFO).
   EXPECT_FALSE(fs::exists(f2));
   q.undo_last();
   EXPECT_TRUE(fs::exists(f2));
   EXPECT_FALSE(fs::exists(f1));

   q.undo_last();
   EXPECT_TRUE(fs::exists(f1));
}

// ─── purge
// ────────────────────────────────────────────────────────────────────

TEST_F(QuarantineTest, PurgeDeletesAllQuarantinedFiles) {
   auto kept = write_file("kept.txt");
   Quarantine q(base_dir_);

   for (int i = 0; i < 4; ++i) {
      auto f = write_file("purge" + std::to_string(i) + ".txt");
      q.quarantine_file(f, kept, 1.0f);
   }

   size_t deleted = q.purge(0);

   EXPECT_EQ(deleted, 4u);
   // Only the manifest should remain.
   size_t remaining = 0;
   for (const auto& e : fs::directory_iterator(base_dir_)) {
      if (e.is_regular_file())
         ++remaining;
   }
   EXPECT_EQ(remaining, 1u); // quarantine.json
}

TEST_F(QuarantineTest, PurgeOnEmptyDirReturnsZero) {
   Quarantine q(base_dir_);
   EXPECT_EQ(q.purge(0), 0u);
}

TEST_F(QuarantineTest, PurgePreservesManifest) {
   auto kept = write_file("kept.txt");
   Quarantine q(base_dir_);
   auto f = write_file("pf.txt");
   q.quarantine_file(f, kept, 1.0f);

   q.purge(0);

   EXPECT_TRUE(fs::exists(base_dir_ / "quarantine.json"));
}

// ─── config integration
// ───────────────────────────────────────────────────────

TEST_F(QuarantineTest, BaseDirIsCorrect) {
   Quarantine q(base_dir_);
   EXPECT_EQ(q.base_dir(), base_dir_);
}

TEST_F(QuarantineTest, LoadManifestOnEmptyDirReturnsEmpty) {
   Quarantine q(base_dir_);
   EXPECT_TRUE(q.load_manifest().empty());
}
