#include "history_manager.hpp"
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <random>
#include <spdlog/spdlog.h>
#include <sstream>

using json = nlohmann::json;

HistoryManager::HistoryManager(const fs::path& db_path)
    : db_(std::make_unique<HistoryDatabase>(db_path)),
      current_session_id_(generate_session_id()) {
   initialized_ = db_->initialize();
   if (!initialized_) {
      spdlog::error("Failed to initialize history database at: {}",
                    db_path.string());
   } else {
      spdlog::info("History database initialized: {}", db_path.string());
   }
}

HistoryManager::~HistoryManager() = default;

// ─── Recording ──────────────────────────────────────────────────────

void HistoryManager::record_move(const fs::path& source, const fs::path& dest,
                                 const std::string& category,
                                 int64_t file_size) {
   if (!initialized_) return;

   HistoryEntry entry;
   entry.timestamp = current_timestamp();
   entry.operation_type = OperationType::MOVE;
   entry.source_path = source.string();
   entry.destination_path = dest.string();
   entry.file_size = file_size;
   entry.file_category = category;
   entry.status = OperationStatus::SUCCESS;
   entry.session_id = current_session_id_;

   db_->record_entry(entry);
}

void HistoryManager::record_quarantine(const fs::path& original,
                                       const fs::path& quarantine,
                                       const fs::path& kept,
                                       float similarity,
                                       const std::string& method) {
   if (!initialized_) return;

   HistoryEntry entry;
   entry.timestamp = current_timestamp();
   entry.operation_type = OperationType::QUARANTINE;
   entry.source_path = original.string();
   entry.destination_path = quarantine.string();
   entry.status = OperationStatus::SUCCESS;
   entry.similarity_score = static_cast<double>(similarity);
   entry.detection_method = method;
   entry.session_id = current_session_id_;

   json meta;
   meta["kept_path"] = kept.string();
   entry.metadata = meta.dump();

   db_->record_entry(entry);
}

void HistoryManager::record_undo_move(const fs::path& source,
                                      const fs::path& dest) {
   if (!initialized_) return;

   HistoryEntry entry;
   entry.timestamp = current_timestamp();
   entry.operation_type = OperationType::UNDO_MOVE;
   entry.source_path = source.string();
   entry.destination_path = dest.string();
   entry.status = OperationStatus::SUCCESS;
   entry.session_id = current_session_id_;

   db_->record_entry(entry);
}

void HistoryManager::record_undo_quarantine(const fs::path& original,
                                            const fs::path& quarantine) {
   if (!initialized_) return;

   HistoryEntry entry;
   entry.timestamp = current_timestamp();
   entry.operation_type = OperationType::UNDO_QUARANTINE;
   entry.source_path = quarantine.string();
   entry.destination_path = original.string();
   entry.status = OperationStatus::SUCCESS;
   entry.session_id = current_session_id_;

   db_->record_entry(entry);
}

void HistoryManager::record_failed(OperationType type, const fs::path& source,
                                   const fs::path& dest,
                                   const std::string& error) {
   if (!initialized_) return;

   HistoryEntry entry;
   entry.timestamp = current_timestamp();
   entry.operation_type = type;
   entry.source_path = source.string();
   entry.destination_path = dest.string();
   entry.status = OperationStatus::FAILED;
   entry.error_message = error;
   entry.session_id = current_session_id_;

   db_->record_entry(entry);
}

// ─── Retrieval ──────────────────────────────────────────────────────

std::vector<HistoryEntry> HistoryManager::get_recent_history(int limit) {
   if (!initialized_) return {};
   return db_->get_entries(limit, 0);
}

std::vector<HistoryEntry> HistoryManager::search(
    const std::string& query, std::optional<OperationType> type,
    std::optional<int> days) {
   if (!initialized_) return {};

   HistoryFilter filter;
   filter.type = type;
   filter.days = days;
   filter.query = query;

   return db_->search_entries(query, filter);
}

// ─── Undo from history ──────────────────────────────────────────────

bool HistoryManager::can_undo_from_history(int64_t entry_id) {
   if (!initialized_) return false;

   auto entry = db_->get_entry(entry_id);
   if (!entry) return false;

   if (entry->status != OperationStatus::SUCCESS) return false;

   // Can only undo MOVE and QUARANTINE operations
   if (entry->operation_type != OperationType::MOVE &&
       entry->operation_type != OperationType::QUARANTINE) {
      return false;
   }

   // Destination must still exist to undo
   return fs::exists(entry->destination_path);
}

bool HistoryManager::undo_operation(int64_t entry_id) {
   if (!initialized_) return false;

   auto entry = db_->get_entry(entry_id);
   if (!entry) return false;

   if (!can_undo_from_history(entry_id)) return false;

   try {
      fs::path source(entry->source_path);
      fs::path dest(entry->destination_path);

      // Recreate source parent directory if needed
      fs::create_directories(source.parent_path());

      // Move file back
      std::error_code ec;
      fs::rename(dest, source, ec);
      if (ec) {
         fs::copy_file(dest, source, fs::copy_options::overwrite_existing, ec);
         if (ec) {
            db_->update_entry_status(entry_id, OperationStatus::FAILED,
                                     ec.message());
            return false;
         }
         fs::remove(dest, ec);
      }

      // Mark original entry as rolled back
      db_->update_entry_status(entry_id, OperationStatus::ROLLED_BACK);

      // Record the undo operation
      if (entry->operation_type == OperationType::MOVE) {
         record_undo_move(dest, source);
      } else {
         record_undo_quarantine(source, dest);
      }

      return true;
   } catch (const std::exception& e) {
      spdlog::error("Failed to undo operation {}: {}", entry_id, e.what());
      db_->update_entry_status(entry_id, OperationStatus::FAILED, e.what());
      return false;
   }
}

// ─── Management ─────────────────────────────────────────────────────

bool HistoryManager::delete_entry(int64_t id) {
   if (!initialized_) return false;
   return db_->delete_entry(id);
}

bool HistoryManager::export_to_json(const fs::path& output_path) {
   if (!initialized_) return false;

   auto entries = db_->get_entries(10000, 0);

   json arr = json::array();
   for (const auto& e : entries) {
      json obj;
      obj["id"] = e.id;
      obj["timestamp"] = e.timestamp;
      obj["operation_type"] = operation_type_to_string(e.operation_type);
      obj["source_path"] = e.source_path;
      obj["destination_path"] = e.destination_path;
      obj["file_size"] = e.file_size;
      obj["file_category"] = e.file_category;
      obj["status"] = operation_status_to_string(e.status);
      obj["error_message"] = e.error_message;
      obj["similarity_score"] = e.similarity_score;
      obj["detection_method"] = e.detection_method;
      obj["session_id"] = e.session_id;
      if (!e.metadata.empty()) {
         try {
            obj["metadata"] = json::parse(e.metadata);
         } catch (...) {
            obj["metadata"] = e.metadata;
         }
      }
      arr.push_back(obj);
   }

   std::ofstream out(output_path);
   if (!out.is_open()) return false;

   out << arr.dump(2);
   return out.good();
}

// ─── Retention ──────────────────────────────────────────────────────

void HistoryManager::set_retention_days(int days) {
   if (!initialized_) return;
   db_->set_setting("retention_days", std::to_string(days));
}

int HistoryManager::get_retention_days() const {
   if (!initialized_) return 30;
   auto val = db_->get_setting("retention_days", "30");
   try {
      return std::stoi(val);
   } catch (...) {
      return 30;
   }
}

size_t HistoryManager::apply_retention_policy() {
   if (!initialized_) return 0;

   int days = get_retention_days();
   if (days <= 0) return 0;

   auto deleted = db_->delete_old_entries(days);
   if (deleted > 0) {
      db_->vacuum();
      spdlog::info("History retention: deleted {} entries older than {} days",
                   deleted, days);
   }
   return deleted;
}

// ─── Stats ──────────────────────────────────────────────────────────

HistorySummary HistoryManager::get_summary() {
   HistorySummary summary;
   if (!initialized_) return summary;

   summary.stats = db_->get_stats();
   summary.retention_days = get_retention_days();

   // Get oldest and newest entries
   auto oldest = db_->get_entries(1, static_cast<int>(summary.stats.total_entries - 1));
   if (!oldest.empty()) {
      summary.oldest_entry_date = oldest.front().timestamp;
   }

   auto newest = db_->get_entries(1, 0);
   if (!newest.empty()) {
      summary.newest_entry_date = newest.front().timestamp;
   }

   return summary;
}

// ─── Private helpers ────────────────────────────────────────────────

std::string HistoryManager::generate_session_id() {
   std::random_device rd;
   std::mt19937 gen(rd());
   std::uniform_int_distribution<uint64_t> dist;

   std::ostringstream oss;
   oss << std::hex << dist(gen) << dist(gen);
   return oss.str();
}

std::string HistoryManager::current_timestamp() {
   const auto now = std::chrono::system_clock::now();
   const std::time_t t = std::chrono::system_clock::to_time_t(now);
   std::tm tm{};
#ifdef _WIN32
   localtime_s(&tm, &t);
#else
   localtime_r(&t, &tm);
#endif
   std::ostringstream oss;
   oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
   return oss.str();
}
