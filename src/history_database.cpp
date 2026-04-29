#include "history_database.hpp"
#include <spdlog/spdlog.h>
#include <sqlite3.h>
#include <sstream>
#include <stdexcept>

// ─── PIMPL implementation ─────────────────────────────────────────

class HistoryDatabase::Impl {
 public:
   explicit Impl(const fs::path& db_path) : db_path_(db_path) {}

   ~Impl() {
      finalize_statements();
      if (db_) {
         sqlite3_close(db_);
         db_ = nullptr;
      }
   }

   bool initialize() {
      fs::create_directories(db_path_.parent_path());

      int rc = sqlite3_open(db_path_.string().c_str(), &db_);
      if (rc != SQLITE_OK) {
         spdlog::error("Failed to open history database: {}",
                       sqlite3_errmsg(db_));
         return false;
      }

      // Enable WAL mode for better concurrent read performance
      exec_sql("PRAGMA journal_mode=WAL;");
      exec_sql("PRAGMA foreign_keys=ON;");

      if (!create_tables()) {
         return false;
      }

      return prepare_statements();
   }

   int64_t record_entry(const HistoryEntry& entry) {
      if (!stmt_insert_) return -1;

      sqlite3_reset(stmt_insert_);
      sqlite3_clear_bindings(stmt_insert_);

      bind_text(stmt_insert_, 1, entry.timestamp);
      bind_text(stmt_insert_, 2, operation_type_to_string(entry.operation_type));
      bind_text(stmt_insert_, 3, entry.source_path);
      bind_text(stmt_insert_, 4, entry.destination_path);
      sqlite3_bind_int64(stmt_insert_, 5, entry.file_size);
      bind_text(stmt_insert_, 6, entry.file_category);
      bind_text(stmt_insert_, 7, operation_status_to_string(entry.status));
      bind_text(stmt_insert_, 8, entry.error_message);
      sqlite3_bind_double(stmt_insert_, 9, entry.similarity_score);
      bind_text(stmt_insert_, 10, entry.detection_method);
      bind_text(stmt_insert_, 11, entry.session_id);
      bind_text(stmt_insert_, 12, entry.metadata);

      int rc = sqlite3_step(stmt_insert_);
      if (rc != SQLITE_DONE) {
         spdlog::error("Failed to insert history entry: {}",
                       sqlite3_errmsg(db_));
         return -1;
      }

      return sqlite3_last_insert_rowid(db_);
   }

   std::optional<HistoryEntry> get_entry(int64_t id) {
      if (!stmt_get_by_id_) return std::nullopt;

      sqlite3_reset(stmt_get_by_id_);
      sqlite3_bind_int64(stmt_get_by_id_, 1, id);

      int rc = sqlite3_step(stmt_get_by_id_);
      if (rc == SQLITE_ROW) {
         return row_to_entry(stmt_get_by_id_);
      }
      return std::nullopt;
   }

   std::vector<HistoryEntry> get_entries(int limit, int offset) {
      if (!stmt_get_all_) return {};

      sqlite3_reset(stmt_get_all_);
      sqlite3_bind_int(stmt_get_all_, 1, limit);
      sqlite3_bind_int(stmt_get_all_, 2, offset);

      std::vector<HistoryEntry> entries;
      while (sqlite3_step(stmt_get_all_) == SQLITE_ROW) {
         entries.push_back(row_to_entry(stmt_get_all_));
      }
      return entries;
   }

   std::vector<HistoryEntry> search_entries(const std::string& query,
                                            const HistoryFilter& filter) {
      std::ostringstream sql;
      sql << "SELECT id, timestamp, operation_type, source_path, "
             "destination_path, file_size, file_category, status, "
             "error_message, similarity_score, detection_method, "
             "session_id, metadata FROM history_entries WHERE 1=1";

      std::vector<std::string> params;

      if (!query.empty()) {
         sql << " AND (source_path LIKE ? OR destination_path LIKE ?)";
         std::string like_query = "%" + query + "%";
         params.push_back(like_query);
         params.push_back(like_query);
      }

      if (filter.type.has_value()) {
         sql << " AND operation_type = ?";
         params.push_back(operation_type_to_string(*filter.type));
      }

      if (filter.status.has_value()) {
         sql << " AND status = ?";
         params.push_back(operation_status_to_string(*filter.status));
      }

      if (filter.days.has_value() && *filter.days > 0) {
         sql << " AND timestamp >= datetime('now', '-" +
                    std::to_string(*filter.days) + " days')";
      }

      sql << " ORDER BY timestamp DESC LIMIT 1000";

      sqlite3_stmt* stmt = nullptr;
      int rc = sqlite3_prepare_v2(db_, sql.str().c_str(), -1, &stmt, nullptr);
      if (rc != SQLITE_OK) {
         spdlog::error("Failed to prepare search query: {}",
                       sqlite3_errmsg(db_));
         return {};
      }

      for (size_t i = 0; i < params.size(); ++i) {
         sqlite3_bind_text(stmt, static_cast<int>(i + 1),
                           params[i].c_str(), -1, SQLITE_TRANSIENT);
      }

      std::vector<HistoryEntry> entries;
      while (sqlite3_step(stmt) == SQLITE_ROW) {
         entries.push_back(row_to_entry(stmt));
      }

      sqlite3_finalize(stmt);
      return entries;
   }

   bool update_entry_status(int64_t id, OperationStatus status,
                            const std::optional<std::string>& error) {
      if (!stmt_update_status_) return false;

      sqlite3_reset(stmt_update_status_);
      bind_text(stmt_update_status_, 1, operation_status_to_string(status));
      bind_text(stmt_update_status_, 2, error.value_or(""));
      sqlite3_bind_int64(stmt_update_status_, 3, id);

      int rc = sqlite3_step(stmt_update_status_);
      return rc == SQLITE_DONE && sqlite3_changes(db_) > 0;
   }

   bool delete_entry(int64_t id) {
      if (!stmt_delete_) return false;

      sqlite3_reset(stmt_delete_);
      sqlite3_bind_int64(stmt_delete_, 1, id);

      int rc = sqlite3_step(stmt_delete_);
      return rc == SQLITE_DONE && sqlite3_changes(db_) > 0;
   }

   size_t delete_old_entries(int days) {
      if (days <= 0) return 0;

      std::string sql =
          "DELETE FROM history_entries WHERE timestamp < datetime('now', '-" +
          std::to_string(days) + " days')";

      char* errmsg = nullptr;
      int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errmsg);
      if (rc != SQLITE_OK) {
         spdlog::error("Failed to delete old entries: {}", errmsg ? errmsg : "");
         sqlite3_free(errmsg);
         return 0;
      }

      return static_cast<size_t>(sqlite3_changes(db_));
   }

   void vacuum() {
      exec_sql("VACUUM;");
   }

   HistoryStats get_stats() {
      HistoryStats stats{};
      const char* sql =
          "SELECT "
          "  COUNT(*) as total, "
          "  SUM(CASE WHEN operation_type='MOVE' THEN 1 ELSE 0 END), "
          "  SUM(CASE WHEN operation_type='QUARANTINE' THEN 1 ELSE 0 END), "
          "  SUM(CASE WHEN operation_type='UNDO_MOVE' THEN 1 ELSE 0 END), "
          "  SUM(CASE WHEN operation_type='UNDO_QUARANTINE' THEN 1 ELSE 0 END), "
          "  SUM(CASE WHEN status='SUCCESS' THEN 1 ELSE 0 END), "
          "  SUM(CASE WHEN status='FAILED' THEN 1 ELSE 0 END), "
          "  SUM(CASE WHEN status='ROLLED_BACK' THEN 1 ELSE 0 END) "
          "FROM history_entries";

      sqlite3_stmt* stmt = nullptr;
      if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
         if (sqlite3_step(stmt) == SQLITE_ROW) {
            stats.total_entries = sqlite3_column_int64(stmt, 0);
            stats.move_count = sqlite3_column_int64(stmt, 1);
            stats.quarantine_count = sqlite3_column_int64(stmt, 2);
            stats.undo_move_count = sqlite3_column_int64(stmt, 3);
            stats.undo_quarantine_count = sqlite3_column_int64(stmt, 4);
            stats.success_count = sqlite3_column_int64(stmt, 5);
            stats.failed_count = sqlite3_column_int64(stmt, 6);
            stats.rolled_back_count = sqlite3_column_int64(stmt, 7);
         }
      }
      sqlite3_finalize(stmt);
      return stats;
   }

   std::string get_setting(const std::string& key,
                           const std::string& default_value) const {
      if (!stmt_get_setting_) return default_value;

      sqlite3_reset(stmt_get_setting_);
      sqlite3_bind_text(stmt_get_setting_, 1, key.c_str(), -1, SQLITE_TRANSIENT);

      if (sqlite3_step(stmt_get_setting_) == SQLITE_ROW) {
         const char* val = reinterpret_cast<const char*>(
             sqlite3_column_text(stmt_get_setting_, 0));
         return val ? std::string(val) : default_value;
      }
      return default_value;
   }

   bool set_setting(const std::string& key, const std::string& value) {
      if (!stmt_set_setting_) return false;

      sqlite3_reset(stmt_set_setting_);
      sqlite3_bind_text(stmt_set_setting_, 1, key.c_str(), -1, SQLITE_TRANSIENT);
      sqlite3_bind_text(stmt_set_setting_, 2, value.c_str(), -1, SQLITE_TRANSIENT);

      return sqlite3_step(stmt_set_setting_) == SQLITE_DONE;
   }

 private:
   fs::path db_path_;
   sqlite3* db_ = nullptr;

   // Prepared statements
   sqlite3_stmt* stmt_insert_ = nullptr;
   sqlite3_stmt* stmt_get_by_id_ = nullptr;
   sqlite3_stmt* stmt_get_all_ = nullptr;
   sqlite3_stmt* stmt_update_status_ = nullptr;
   sqlite3_stmt* stmt_delete_ = nullptr;
   sqlite3_stmt* stmt_get_setting_ = nullptr;
   sqlite3_stmt* stmt_set_setting_ = nullptr;

   bool create_tables() {
      const char* schema = R"SQL(
         CREATE TABLE IF NOT EXISTS history_entries (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp TEXT NOT NULL,
            operation_type TEXT NOT NULL,
            source_path TEXT NOT NULL,
            destination_path TEXT,
            file_size INTEGER,
            file_category TEXT,
            status TEXT NOT NULL,
            error_message TEXT,
            similarity_score REAL,
            detection_method TEXT,
            session_id TEXT,
            metadata TEXT
         );

         CREATE INDEX IF NOT EXISTS idx_history_timestamp
            ON history_entries(timestamp);
         CREATE INDEX IF NOT EXISTS idx_history_type
            ON history_entries(operation_type);
         CREATE INDEX IF NOT EXISTS idx_history_status
            ON history_entries(status);
         CREATE INDEX IF NOT EXISTS idx_history_source
            ON history_entries(source_path);
         CREATE INDEX IF NOT EXISTS idx_history_session
            ON history_entries(session_id);

         CREATE TABLE IF NOT EXISTS history_settings (
            key TEXT PRIMARY KEY,
            value TEXT NOT NULL
         );

         INSERT OR IGNORE INTO history_settings (key, value)
         VALUES ('retention_days', '30');
      )SQL";

      char* errmsg = nullptr;
      int rc = sqlite3_exec(db_, schema, nullptr, nullptr, &errmsg);
      if (rc != SQLITE_OK) {
         spdlog::error("Failed to create history tables: {}",
                       errmsg ? errmsg : "");
         sqlite3_free(errmsg);
         return false;
      }
      return true;
   }

   bool prepare_statements() {
      auto prep = [this](const char* sql, sqlite3_stmt** stmt) -> bool {
         int rc = sqlite3_prepare_v2(db_, sql, -1, stmt, nullptr);
         if (rc != SQLITE_OK) {
            spdlog::error("Failed to prepare statement: {}",
                          sqlite3_errmsg(db_));
            return false;
         }
         return true;
      };

      bool ok = true;
      ok &= prep(
          "INSERT INTO history_entries "
          "(timestamp, operation_type, source_path, destination_path, "
          "file_size, file_category, status, error_message, "
          "similarity_score, detection_method, session_id, metadata) "
          "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
          &stmt_insert_);

      ok &= prep(
          "SELECT id, timestamp, operation_type, source_path, "
          "destination_path, file_size, file_category, status, "
          "error_message, similarity_score, detection_method, "
          "session_id, metadata FROM history_entries WHERE id = ?",
          &stmt_get_by_id_);

      ok &= prep(
          "SELECT id, timestamp, operation_type, source_path, "
          "destination_path, file_size, file_category, status, "
          "error_message, similarity_score, detection_method, "
          "session_id, metadata FROM history_entries "
          "ORDER BY timestamp DESC LIMIT ? OFFSET ?",
          &stmt_get_all_);

      ok &= prep(
          "UPDATE history_entries SET status = ?, error_message = ? "
          "WHERE id = ?",
          &stmt_update_status_);

      ok &= prep(
          "DELETE FROM history_entries WHERE id = ?",
          &stmt_delete_);

      ok &= prep(
          "SELECT value FROM history_settings WHERE key = ?",
          &stmt_get_setting_);

      ok &= prep(
          "INSERT OR REPLACE INTO history_settings (key, value) VALUES (?, ?)",
          &stmt_set_setting_);

      return ok;
   }

   void finalize_statements() {
      auto fin = [](sqlite3_stmt*& s) {
         if (s) {
            sqlite3_finalize(s);
            s = nullptr;
         }
      };
      fin(stmt_insert_);
      fin(stmt_get_by_id_);
      fin(stmt_get_all_);
      fin(stmt_update_status_);
      fin(stmt_delete_);
      fin(stmt_get_setting_);
      fin(stmt_set_setting_);
   }

   void exec_sql(const char* sql) {
      char* errmsg = nullptr;
      int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &errmsg);
      if (rc != SQLITE_OK) {
         spdlog::error("SQL exec failed: {}", errmsg ? errmsg : "");
         sqlite3_free(errmsg);
      }
   }

   static void bind_text(sqlite3_stmt* stmt, int col, const std::string& val) {
      sqlite3_bind_text(stmt, col, val.c_str(), -1, SQLITE_TRANSIENT);
   }

   static HistoryEntry row_to_entry(sqlite3_stmt* stmt) {
      HistoryEntry e;
      e.id = sqlite3_column_int64(stmt, 0);

      auto col_text = [&](int col) -> std::string {
         const char* p = reinterpret_cast<const char*>(
             sqlite3_column_text(stmt, col));
         return p ? std::string(p) : "";
      };

      e.timestamp = col_text(1);
      e.operation_type = string_to_operation_type(col_text(2));
      e.source_path = col_text(3);
      e.destination_path = col_text(4);
      e.file_size = sqlite3_column_int64(stmt, 5);
      e.file_category = col_text(6);
      e.status = string_to_operation_status(col_text(7));
      e.error_message = col_text(8);
      e.similarity_score = sqlite3_column_double(stmt, 9);
      e.detection_method = col_text(10);
      e.session_id = col_text(11);
      e.metadata = col_text(12);
      return e;
   }
};

// ─── HistoryDatabase public methods (delegate to Impl) ─────────────

HistoryDatabase::HistoryDatabase(const fs::path& db_path)
    : impl_(std::make_unique<Impl>(db_path)) {}

HistoryDatabase::~HistoryDatabase() = default;

bool HistoryDatabase::initialize() { return impl_->initialize(); }

int64_t HistoryDatabase::record_entry(const HistoryEntry& entry) {
   return impl_->record_entry(entry);
}

std::optional<HistoryEntry> HistoryDatabase::get_entry(int64_t id) {
   return impl_->get_entry(id);
}

std::vector<HistoryEntry> HistoryDatabase::get_entries(int limit, int offset) {
   return impl_->get_entries(limit, offset);
}

std::vector<HistoryEntry> HistoryDatabase::search_entries(
    const std::string& query, const HistoryFilter& filter) {
   return impl_->search_entries(query, filter);
}

bool HistoryDatabase::update_entry_status(
    int64_t id, OperationStatus status,
    const std::optional<std::string>& error) {
   return impl_->update_entry_status(id, status, error);
}

bool HistoryDatabase::delete_entry(int64_t id) {
   return impl_->delete_entry(id);
}

size_t HistoryDatabase::delete_old_entries(int days) {
   return impl_->delete_old_entries(days);
}

void HistoryDatabase::vacuum() { impl_->vacuum(); }

HistoryStats HistoryDatabase::get_stats() { return impl_->get_stats(); }

std::string HistoryDatabase::get_setting(const std::string& key,
                                         const std::string& default_value) const {
   return impl_->get_setting(key, default_value);
}

bool HistoryDatabase::set_setting(const std::string& key,
                                  const std::string& value) {
   return impl_->set_setting(key, value);
}
