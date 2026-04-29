#include "history_manager.hpp"
#include "history_entry.hpp"
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

class HistoryManagerTest : public ::testing::Test {
 protected:
   void SetUp() override {
      test_dir_ = fs::temp_directory_path() / "history_mgr_test";
      fs::create_directories(test_dir_);
      db_path_ = test_dir_ / "test_history.db";

      // Create some test files for undo testing
      test_src_dir_ = test_dir_ / "source";
      test_dst_dir_ = test_dir_ / "destination";
      fs::create_directories(test_src_dir_);
      fs::create_directories(test_dst_dir_);
   }

   void TearDown() override {
      fs::remove_all(test_dir_);
   }

   void create_test_file(const fs::path& path, const std::string& content = "test data") {
      fs::create_directories(path.parent_path());
      std::ofstream f(path);
      f << content;
   }

   fs::path test_dir_;
   fs::path db_path_;
   fs::path test_src_dir_;
   fs::path test_dst_dir_;
};

TEST_F(HistoryManagerTest, InitializesSuccessfully) {
   HistoryManager mgr(db_path_);
   EXPECT_TRUE(mgr.is_initialized());
}

TEST_F(HistoryManagerTest, RecordMoveCreatesEntry) {
   HistoryManager mgr(db_path_);
   ASSERT_TRUE(mgr.is_initialized());

   fs::path src = test_src_dir_ / "report.pdf";
   fs::path dst = test_dst_dir_ / "Documents" / "report.pdf";

   mgr.record_move(src, dst, "documents", 1024);

   auto history = mgr.get_recent_history(10);
   ASSERT_EQ(history.size(), 1u);
   EXPECT_EQ(history[0].operation_type, OperationType::MOVE);
   EXPECT_EQ(history[0].source_path, src.string());
   EXPECT_EQ(history[0].destination_path, dst.string());
   EXPECT_EQ(history[0].file_category, "documents");
   EXPECT_EQ(history[0].file_size, 1024);
   EXPECT_EQ(history[0].status, OperationStatus::SUCCESS);
}

TEST_F(HistoryManagerTest, RecordQuarantineWithSimilarity) {
   HistoryManager mgr(db_path_);
   ASSERT_TRUE(mgr.is_initialized());

   fs::path original = test_src_dir_ / "photo.jpg";
   fs::path quarantine = test_dir_ / "quarantine" / "photo.jpg";
   fs::path kept = test_src_dir_ / "photo_original.jpg";

   mgr.record_quarantine(original, quarantine, kept, 0.95f, "simhash");

   auto history = mgr.get_recent_history(10);
   ASSERT_EQ(history.size(), 1u);
   EXPECT_EQ(history[0].operation_type, OperationType::QUARANTINE);
   EXPECT_NEAR(history[0].similarity_score, 0.95, 0.01);
   EXPECT_EQ(history[0].detection_method, "simhash");
   EXPECT_FALSE(history[0].metadata.empty());
}

TEST_F(HistoryManagerTest, RecordUndoMove) {
   HistoryManager mgr(db_path_);
   ASSERT_TRUE(mgr.is_initialized());

   fs::path src = test_src_dir_ / "file.txt";
   fs::path dst = test_dst_dir_ / "file.txt";

   mgr.record_undo_move(src, dst);

   auto history = mgr.get_recent_history(10);
   ASSERT_EQ(history.size(), 1u);
   EXPECT_EQ(history[0].operation_type, OperationType::UNDO_MOVE);
}

TEST_F(HistoryManagerTest, RecordUndoQuarantine) {
   HistoryManager mgr(db_path_);
   ASSERT_TRUE(mgr.is_initialized());

   mgr.record_undo_quarantine(test_src_dir_ / "a.txt",
                              test_dst_dir_ / "b.txt");

   auto history = mgr.get_recent_history(10);
   ASSERT_EQ(history.size(), 1u);
   EXPECT_EQ(history[0].operation_type, OperationType::UNDO_QUARANTINE);
}

TEST_F(HistoryManagerTest, RecordFailedEntry) {
   HistoryManager mgr(db_path_);
   ASSERT_TRUE(mgr.is_initialized());

   mgr.record_failed(OperationType::MOVE, test_src_dir_ / "missing.txt",
                      test_dst_dir_ / "missing.txt", "File not found");

   auto history = mgr.get_recent_history(10);
   ASSERT_EQ(history.size(), 1u);
   EXPECT_EQ(history[0].status, OperationStatus::FAILED);
   EXPECT_EQ(history[0].error_message, "File not found");
}

TEST_F(HistoryManagerTest, UndoFromHistoryRestoresFile) {
   HistoryManager mgr(db_path_);
   ASSERT_TRUE(mgr.is_initialized());

   // Create a file at the destination (simulating a previous move)
   fs::path src = test_src_dir_ / "moved_file.txt";
   fs::path dst = test_dst_dir_ / "moved_file.txt";
   create_test_file(dst, "original content");

   // Record the original move
   mgr.record_move(src, dst, "documents", 16);

   auto history = mgr.get_recent_history(1);
   ASSERT_EQ(history.size(), 1u);
   int64_t entry_id = history[0].id;

   // Undo it
   ASSERT_TRUE(mgr.can_undo_from_history(entry_id));
   ASSERT_TRUE(mgr.undo_operation(entry_id));

   // File should be moved back
   EXPECT_TRUE(fs::exists(src));
   EXPECT_FALSE(fs::exists(dst));

   // The original entry should be marked as ROLLED_BACK
   auto updated = mgr.get_recent_history(10);
   bool found_rolled_back = false;
   bool found_undo = false;
   for (const auto& e : updated) {
      if (e.id == entry_id) {
         EXPECT_EQ(e.status, OperationStatus::ROLLED_BACK);
         found_rolled_back = true;
      }
      if (e.operation_type == OperationType::UNDO_MOVE) {
         found_undo = true;
      }
   }
   EXPECT_TRUE(found_rolled_back);
   EXPECT_TRUE(found_undo);
}

TEST_F(HistoryManagerTest, UndoFromHistoryRecordsUndoOperation) {
   HistoryManager mgr(db_path_);
   ASSERT_TRUE(mgr.is_initialized());

   fs::path src = test_src_dir_ / "file.txt";
   fs::path dst = test_dst_dir_ / "file.txt";
   create_test_file(dst);

   mgr.record_move(src, dst, "documents", 8);
   auto history = mgr.get_recent_history(1);
   mgr.undo_operation(history[0].id);

   auto all = mgr.get_recent_history(10);
   ASSERT_GE(all.size(), 2u);

   bool found_undo = false;
   for (const auto& e : all) {
      if (e.operation_type == OperationType::UNDO_MOVE) {
         found_undo = true;
      }
   }
   EXPECT_TRUE(found_undo);
}

TEST_F(HistoryManagerTest, CannotUndoFailedEntry) {
   HistoryManager mgr(db_path_);
   ASSERT_TRUE(mgr.is_initialized());

   mgr.record_failed(OperationType::MOVE, test_src_dir_ / "x.txt",
                      test_dst_dir_ / "x.txt", "Error");

   auto history = mgr.get_recent_history(1);
   EXPECT_FALSE(mgr.can_undo_from_history(history[0].id));
}

TEST_F(HistoryManagerTest, CannotUndoUndoEntry) {
   HistoryManager mgr(db_path_);
   ASSERT_TRUE(mgr.is_initialized());

   mgr.record_undo_move(test_src_dir_ / "x.txt", test_dst_dir_ / "x.txt");

   auto history = mgr.get_recent_history(1);
   EXPECT_FALSE(mgr.can_undo_from_history(history[0].id));
}

TEST_F(HistoryManagerTest, SearchFiltersByType) {
   HistoryManager mgr(db_path_);
   ASSERT_TRUE(mgr.is_initialized());

   mgr.record_move(test_src_dir_ / "a.txt", test_dst_dir_ / "a.txt",
                    "documents", 100);
   mgr.record_quarantine(test_src_dir_ / "b.jpg", test_dst_dir_ / "b.jpg",
                          test_src_dir_ / "c.jpg", 0.99f, "hash");

   auto results = mgr.search("", OperationType::MOVE);
   ASSERT_EQ(results.size(), 1u);
   EXPECT_EQ(results[0].operation_type, OperationType::MOVE);
}

TEST_F(HistoryManagerTest, SearchByQueryString) {
   HistoryManager mgr(db_path_);
   ASSERT_TRUE(mgr.is_initialized());

   mgr.record_move(test_src_dir_ / "report.pdf", test_dst_dir_ / "report.pdf",
                    "documents", 100);
   mgr.record_move(test_src_dir_ / "photo.jpg", test_dst_dir_ / "photo.jpg",
                    "images", 200);

   auto results = mgr.search("report");
   ASSERT_EQ(results.size(), 1u);
   EXPECT_NE(results[0].source_path.find("report"), std::string::npos);
}

TEST_F(HistoryManagerTest, RetentionPolicyDefaults) {
   HistoryManager mgr(db_path_);
   ASSERT_TRUE(mgr.is_initialized());

   EXPECT_EQ(mgr.get_retention_days(), 30);
}

TEST_F(HistoryManagerTest, SetAndGetRetentionDays) {
   HistoryManager mgr(db_path_);
   ASSERT_TRUE(mgr.is_initialized());

   mgr.set_retention_days(60);
   EXPECT_EQ(mgr.get_retention_days(), 60);

   mgr.set_retention_days(0);
   EXPECT_EQ(mgr.get_retention_days(), 0);
}

TEST_F(HistoryManagerTest, RetentionPolicyDeletesOldEntries) {
   HistoryManager mgr(db_path_);
   ASSERT_TRUE(mgr.is_initialized());

   mgr.set_retention_days(1);

   // This entry is recent, should survive
   mgr.record_move(test_src_dir_ / "new.txt", test_dst_dir_ / "new.txt",
                    "documents", 100);

   size_t deleted = mgr.apply_retention_policy();
   // No old entries to delete
   EXPECT_EQ(deleted, 0u);

   // Recent entries survive
   auto history = mgr.get_recent_history(10);
   EXPECT_EQ(history.size(), 1u);
}

TEST_F(HistoryManagerTest, DeleteEntry) {
   HistoryManager mgr(db_path_);
   ASSERT_TRUE(mgr.is_initialized());

   mgr.record_move(test_src_dir_ / "a.txt", test_dst_dir_ / "a.txt",
                    "documents", 100);

   auto history = mgr.get_recent_history(1);
   ASSERT_EQ(history.size(), 1u);

   EXPECT_TRUE(mgr.delete_entry(history[0].id));

   history = mgr.get_recent_history(1);
   EXPECT_EQ(history.size(), 0u);
}

TEST_F(HistoryManagerTest, ExportToJsonCreatesValidFile) {
   HistoryManager mgr(db_path_);
   ASSERT_TRUE(mgr.is_initialized());

   mgr.record_move(test_src_dir_ / "a.txt", test_dst_dir_ / "a.txt",
                    "documents", 100);
   mgr.record_quarantine(test_src_dir_ / "b.jpg", test_dst_dir_ / "b.jpg",
                          test_src_dir_ / "c.jpg", 0.95f, "hash");

   fs::path export_path = test_dir_ / "export.json";
   ASSERT_TRUE(mgr.export_to_json(export_path));
   ASSERT_TRUE(fs::exists(export_path));

   std::ifstream f(export_path);
   nlohmann::json j;
   EXPECT_NO_THROW(f >> j);
   ASSERT_TRUE(j.is_array());
   EXPECT_EQ(j.size(), 2u);
   EXPECT_EQ(j[0]["operation_type"], "QUARANTINE");
   EXPECT_EQ(j[1]["operation_type"], "MOVE");
}

TEST_F(HistoryManagerTest, GetSummaryReturnsCorrectData) {
   HistoryManager mgr(db_path_);
   ASSERT_TRUE(mgr.is_initialized());

   mgr.record_move(test_src_dir_ / "a.txt", test_dst_dir_ / "a.txt",
                    "documents", 100);
   mgr.record_move(test_src_dir_ / "b.txt", test_dst_dir_ / "b.txt",
                    "documents", 200);

   auto summary = mgr.get_summary();
   EXPECT_EQ(summary.stats.total_entries, 2);
   EXPECT_EQ(summary.stats.move_count, 2);
   EXPECT_EQ(summary.retention_days, 30);
}

TEST_F(HistoryManagerTest, HistorySurvivesRestart) {
   // Create manager, record some entries, destroy it
   {
      HistoryManager mgr(db_path_);
      ASSERT_TRUE(mgr.is_initialized());

      mgr.record_move(test_src_dir_ / "a.txt", test_dst_dir_ / "a.txt",
                       "documents", 100);
      mgr.record_quarantine(test_src_dir_ / "b.jpg", test_dst_dir_ / "b.jpg",
                             test_src_dir_ / "c.jpg", 0.9f, "hash");
   }

   // Create new manager from same DB - data should persist
   {
      HistoryManager mgr(db_path_);
      ASSERT_TRUE(mgr.is_initialized());

      auto history = mgr.get_recent_history(10);
      ASSERT_EQ(history.size(), 2u);
   }
}
