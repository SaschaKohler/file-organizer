#pragma once

#include "history_entry.hpp"
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace fs = std::filesystem;

class HistoryDatabase {
 public:
   explicit HistoryDatabase(const fs::path& db_path);
   ~HistoryDatabase();

   HistoryDatabase(const HistoryDatabase&) = delete;
   HistoryDatabase& operator=(const HistoryDatabase&) = delete;

   bool initialize();

   // CRUD
   int64_t record_entry(const HistoryEntry& entry);
   std::optional<HistoryEntry> get_entry(int64_t id);
   std::vector<HistoryEntry> get_entries(int limit = 100, int offset = 0);
   std::vector<HistoryEntry> search_entries(const std::string& query,
                                            const HistoryFilter& filter);
   bool update_entry_status(int64_t id, OperationStatus status,
                            const std::optional<std::string>& error = std::nullopt);
   bool delete_entry(int64_t id);

   // Maintenance
   size_t delete_old_entries(int days);
   void vacuum();

   // Stats
   HistoryStats get_stats();

   // Settings
   std::string get_setting(const std::string& key,
                           const std::string& default_value = "") const;
   bool set_setting(const std::string& key, const std::string& value);

 private:
   class Impl;
   std::unique_ptr<Impl> impl_;
};
