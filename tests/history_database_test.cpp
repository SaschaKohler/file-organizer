#include "history_database.hpp"
#include "history_entry.hpp"
#include <gtest/gtest.h>
#include <filesystem>
#include <thread>

namespace fs = std::filesystem;

class HistoryDatabaseTest : public ::testing::Test {
 protected:
   void SetUp() override {
      test_dir_ = fs::temp_directory_path() / "history_db_test";
      fs::create_directories(test_dir_);
      db_path_ = test_dir_ / "test_history.db";
   }

   void TearDown() override {
      fs::remove_all(test_dir_);
   }

   HistoryEntry make_move_entry() {
      HistoryEntry e;
      e.timestamp = "2026-04-29T10:00:00";
      e.operation_type = OperationType::MOVE;
      e.source_path = "/home/user/Downloads/report.pdf";
      e.destination_path = "/home/user/Organized/Documents/report.pdf";
      e.file_size = 1024;
      e.file_category = "documents";
      e.status = OperationStatus::SUCCESS;
      e.session_id = "test-session-1";
      return e;
   }

   HistoryEntry make_quarantine_entry() {
      HistoryEntry e;
      e.timestamp = "2026-04-29T10:05:00";
      e.operation_type = OperationType::QUARANTINE;
      e.source_path = "/home/user/Photos/IMG_001.jpg";
      e.destination_path = "/home/user/.quarantine/IMG_001.jpg";
      e.file_size = 2048;
      e.file_category = "images";
      e.status = OperationStatus::SUCCESS;
      e.similarity_score = 0.98;
      e.detection_method = "hash";
      e.session_id = "test-session-1";
      e.metadata = R"({"kept_path":"/home/user/Photos/IMG_002.jpg"})";
      return e;
   }

   fs::path test_dir_;
   fs::path db_path_;
};

TEST_F(HistoryDatabaseTest, InitializeCreatesTables) {
   HistoryDatabase db(db_path_);
   ASSERT_TRUE(db.initialize());
   EXPECT_TRUE(fs::exists(db_path_));
}

TEST_F(HistoryDatabaseTest, InitializeTwiceIsIdempotent) {
   HistoryDatabase db(db_path_);
   ASSERT_TRUE(db.initialize());
   ASSERT_TRUE(db.initialize());
}

TEST_F(HistoryDatabaseTest, RecordAndRetrieveEntry) {
   HistoryDatabase db(db_path_);
   ASSERT_TRUE(db.initialize());

   auto entry = make_move_entry();
   int64_t id = db.record_entry(entry);
   ASSERT_GT(id, 0);

   auto retrieved = db.get_entry(id);
   ASSERT_TRUE(retrieved.has_value());
   EXPECT_EQ(retrieved->id, id);
   EXPECT_EQ(retrieved->timestamp, entry.timestamp);
   EXPECT_EQ(retrieved->operation_type, OperationType::MOVE);
   EXPECT_EQ(retrieved->source_path, entry.source_path);
   EXPECT_EQ(retrieved->destination_path, entry.destination_path);
   EXPECT_EQ(retrieved->file_size, entry.file_size);
   EXPECT_EQ(retrieved->file_category, "documents");
   EXPECT_EQ(retrieved->status, OperationStatus::SUCCESS);
   EXPECT_EQ(retrieved->session_id, "test-session-1");
}

TEST_F(HistoryDatabaseTest, GetEntryReturnsNulloptForMissingId) {
   HistoryDatabase db(db_path_);
   ASSERT_TRUE(db.initialize());

   auto result = db.get_entry(999);
   EXPECT_FALSE(result.has_value());
}

TEST_F(HistoryDatabaseTest, GetEntriesReturnsInReverseChronologicalOrder) {
   HistoryDatabase db(db_path_);
   ASSERT_TRUE(db.initialize());

   auto e1 = make_move_entry();
   e1.timestamp = "2026-04-29T09:00:00";
   db.record_entry(e1);

   auto e2 = make_move_entry();
   e2.timestamp = "2026-04-29T10:00:00";
   db.record_entry(e2);

   auto e3 = make_quarantine_entry();
   e3.timestamp = "2026-04-29T11:00:00";
   db.record_entry(e3);

   auto entries = db.get_entries(10, 0);
   ASSERT_EQ(entries.size(), 3u);
   EXPECT_EQ(entries[0].timestamp, "2026-04-29T11:00:00");
   EXPECT_EQ(entries[1].timestamp, "2026-04-29T10:00:00");
   EXPECT_EQ(entries[2].timestamp, "2026-04-29T09:00:00");
}

TEST_F(HistoryDatabaseTest, GetEntriesWithLimitAndOffset) {
   HistoryDatabase db(db_path_);
   ASSERT_TRUE(db.initialize());

   for (int i = 0; i < 5; ++i) {
      auto e = make_move_entry();
      e.timestamp = "2026-04-29T1" + std::to_string(i) + ":00:00";
      db.record_entry(e);
   }

   auto page1 = db.get_entries(2, 0);
   EXPECT_EQ(page1.size(), 2u);

   auto page2 = db.get_entries(2, 2);
   EXPECT_EQ(page2.size(), 2u);

   auto page3 = db.get_entries(2, 4);
   EXPECT_EQ(page3.size(), 1u);
}

TEST_F(HistoryDatabaseTest, SearchByPath) {
   HistoryDatabase db(db_path_);
   ASSERT_TRUE(db.initialize());

   auto e1 = make_move_entry();
   e1.source_path = "/home/user/Downloads/report.pdf";
   e1.destination_path = "/home/user/Organized/Documents/report.pdf";
   db.record_entry(e1);

   auto e2 = make_move_entry();
   e2.source_path = "/home/user/Downloads/photo.jpg";
   e2.destination_path = "/home/user/Organized/Images/photo.jpg";
   db.record_entry(e2);

   HistoryFilter filter;
   auto results = db.search_entries("report", filter);
   ASSERT_EQ(results.size(), 1u);
   EXPECT_NE(results[0].source_path.find("report"), std::string::npos);
}

TEST_F(HistoryDatabaseTest, SearchByType) {
   HistoryDatabase db(db_path_);
   ASSERT_TRUE(db.initialize());

   db.record_entry(make_move_entry());
   db.record_entry(make_quarantine_entry());

   HistoryFilter filter;
   filter.type = OperationType::QUARANTINE;
   auto results = db.search_entries("", filter);
   ASSERT_EQ(results.size(), 1u);
   EXPECT_EQ(results[0].operation_type, OperationType::QUARANTINE);
}

TEST_F(HistoryDatabaseTest, UpdateEntryStatus) {
   HistoryDatabase db(db_path_);
   ASSERT_TRUE(db.initialize());

   int64_t id = db.record_entry(make_move_entry());
   ASSERT_GT(id, 0);

   ASSERT_TRUE(db.update_entry_status(id, OperationStatus::ROLLED_BACK));

   auto entry = db.get_entry(id);
   ASSERT_TRUE(entry.has_value());
   EXPECT_EQ(entry->status, OperationStatus::ROLLED_BACK);
}

TEST_F(HistoryDatabaseTest, UpdateEntryStatusWithError) {
   HistoryDatabase db(db_path_);
   ASSERT_TRUE(db.initialize());

   int64_t id = db.record_entry(make_move_entry());
   ASSERT_TRUE(
       db.update_entry_status(id, OperationStatus::FAILED, "File not found"));

   auto entry = db.get_entry(id);
   ASSERT_TRUE(entry.has_value());
   EXPECT_EQ(entry->status, OperationStatus::FAILED);
   EXPECT_EQ(entry->error_message, "File not found");
}

TEST_F(HistoryDatabaseTest, DeleteEntry) {
   HistoryDatabase db(db_path_);
   ASSERT_TRUE(db.initialize());

   int64_t id = db.record_entry(make_move_entry());
   ASSERT_TRUE(db.delete_entry(id));
   EXPECT_FALSE(db.get_entry(id).has_value());
}

TEST_F(HistoryDatabaseTest, DeleteNonexistentReturnsFalse) {
   HistoryDatabase db(db_path_);
   ASSERT_TRUE(db.initialize());

   EXPECT_FALSE(db.delete_entry(999));
}

TEST_F(HistoryDatabaseTest, DeleteOldEntries) {
   HistoryDatabase db(db_path_);
   ASSERT_TRUE(db.initialize());

   // Insert an entry with a very old timestamp
   HistoryEntry old_entry = make_move_entry();
   old_entry.timestamp = "2020-01-01T00:00:00";
   db.record_entry(old_entry);

   // Insert a recent entry
   HistoryEntry recent_entry = make_move_entry();
   recent_entry.timestamp = "2026-04-29T10:00:00";
   db.record_entry(recent_entry);

   // Delete entries older than 30 days
   size_t deleted = db.delete_old_entries(30);
   EXPECT_EQ(deleted, 1u);

   auto entries = db.get_entries(100, 0);
   EXPECT_EQ(entries.size(), 1u);
}

TEST_F(HistoryDatabaseTest, VacuumDoesNotThrow) {
   HistoryDatabase db(db_path_);
   ASSERT_TRUE(db.initialize());
   EXPECT_NO_THROW(db.vacuum());
}

TEST_F(HistoryDatabaseTest, GetStatsCountsCorrectly) {
   HistoryDatabase db(db_path_);
   ASSERT_TRUE(db.initialize());

   db.record_entry(make_move_entry());
   db.record_entry(make_move_entry());
   db.record_entry(make_quarantine_entry());

   auto stats = db.get_stats();
   EXPECT_EQ(stats.total_entries, 3);
   EXPECT_EQ(stats.move_count, 2);
   EXPECT_EQ(stats.quarantine_count, 1);
   EXPECT_EQ(stats.success_count, 3);
   EXPECT_EQ(stats.failed_count, 0);
}

TEST_F(HistoryDatabaseTest, SettingsDefaultRetention) {
   HistoryDatabase db(db_path_);
   ASSERT_TRUE(db.initialize());

   auto val = db.get_setting("retention_days", "0");
   EXPECT_EQ(val, "30");
}

TEST_F(HistoryDatabaseTest, SetAndGetSetting) {
   HistoryDatabase db(db_path_);
   ASSERT_TRUE(db.initialize());

   ASSERT_TRUE(db.set_setting("retention_days", "60"));
   EXPECT_EQ(db.get_setting("retention_days"), "60");
}

TEST_F(HistoryDatabaseTest, ConcurrentWrites) {
   HistoryDatabase db(db_path_);
   ASSERT_TRUE(db.initialize());

   // Write multiple entries sequentially (same connection)
   for (int i = 0; i < 50; ++i) {
      auto e = make_move_entry();
      e.timestamp = "2026-04-29T" + std::to_string(10 + (i / 60)) + ":" +
                    (i < 10 ? "0" : "") + std::to_string(i % 60) + ":00";
      EXPECT_GT(db.record_entry(e), 0);
   }

   auto stats = db.get_stats();
   EXPECT_EQ(stats.total_entries, 50);
}

TEST_F(HistoryDatabaseTest, QuarantineEntryPreservesSimilarity) {
   HistoryDatabase db(db_path_);
   ASSERT_TRUE(db.initialize());

   auto entry = make_quarantine_entry();
   int64_t id = db.record_entry(entry);

   auto retrieved = db.get_entry(id);
   ASSERT_TRUE(retrieved.has_value());
   EXPECT_NEAR(retrieved->similarity_score, 0.98, 0.001);
   EXPECT_EQ(retrieved->detection_method, "hash");
}
