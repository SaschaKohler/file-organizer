#pragma once

#include "history_database.hpp"
#include "history_entry.hpp"
#include <chrono>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace fs = std::filesystem;

struct FileInfo;

struct HistorySummary {
   HistoryStats stats;
   int retention_days = 30;
   std::string oldest_entry_date;
   std::string newest_entry_date;
};

class HistoryManager {
 public:
   explicit HistoryManager(const fs::path& db_path);
   ~HistoryManager();

   HistoryManager(const HistoryManager&) = delete;
   HistoryManager& operator=(const HistoryManager&) = delete;

   [[nodiscard]] bool is_initialized() const { return initialized_; }

   // Recording (called from Organizer/Quarantine)
   void record_move(const fs::path& source, const fs::path& dest,
                    const std::string& category, int64_t file_size);
   void record_quarantine(const fs::path& original, const fs::path& quarantine,
                          const fs::path& kept, float similarity,
                          const std::string& method);
   void record_undo_move(const fs::path& source, const fs::path& dest);
   void record_undo_quarantine(const fs::path& original,
                               const fs::path& quarantine);
   void record_failed(OperationType type, const fs::path& source,
                      const fs::path& dest, const std::string& error);

   // Retrieval
   std::vector<HistoryEntry> get_recent_history(int limit = 100);
   std::vector<HistoryEntry> search(
       const std::string& query,
       std::optional<OperationType> type = std::nullopt,
       std::optional<int> days = std::nullopt);

   // Undo from history
   bool can_undo_from_history(int64_t entry_id);
   bool undo_operation(int64_t entry_id);

   // Management
   bool delete_entry(int64_t id);
   bool export_to_json(const fs::path& output_path);

   // Retention
   void set_retention_days(int days);
   int get_retention_days() const;
   size_t apply_retention_policy();

   // Stats
   HistorySummary get_summary();

 private:
   std::unique_ptr<HistoryDatabase> db_;
   std::string current_session_id_;
   bool initialized_ = false;

   std::string generate_session_id();
   std::string current_timestamp();
};
