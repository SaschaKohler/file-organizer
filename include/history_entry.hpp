#pragma once

#include <cstdint>
#include <optional>
#include <string>

enum class OperationType {
   MOVE,
   QUARANTINE,
   UNDO_MOVE,
   UNDO_QUARANTINE
};

enum class OperationStatus {
   SUCCESS,
   FAILED,
   PENDING,
   ROLLED_BACK
};

struct HistoryEntry {
   int64_t id = 0;
   std::string timestamp;           // ISO-8601 format
   OperationType operation_type;
   std::string source_path;
   std::string destination_path;
   int64_t file_size = 0;           // Bytes
   std::string file_category;       // Document, Image, etc.
   OperationStatus status = OperationStatus::SUCCESS;
   std::string error_message;       // If status is FAILED
   double similarity_score = 0.0;   // For quarantine operations (0.0-1.0)
   std::string detection_method;    // "hash", "simhash"
   std::string session_id;          // Group operations by session
   std::string metadata;            // JSON blob for extensibility
};

struct HistoryFilter {
   std::optional<OperationType> type;
   std::optional<OperationStatus> status;
   std::optional<int> days;         // Only entries within N days
   std::string query;               // Search in paths
};

struct HistoryStats {
   int64_t total_entries = 0;
   int64_t move_count = 0;
   int64_t quarantine_count = 0;
   int64_t undo_move_count = 0;
   int64_t undo_quarantine_count = 0;
   int64_t success_count = 0;
   int64_t failed_count = 0;
   int64_t rolled_back_count = 0;
};

inline std::string operation_type_to_string(OperationType type) {
   switch (type) {
   case OperationType::MOVE: return "MOVE";
   case OperationType::QUARANTINE: return "QUARANTINE";
   case OperationType::UNDO_MOVE: return "UNDO_MOVE";
   case OperationType::UNDO_QUARANTINE: return "UNDO_QUARANTINE";
   }
   return "UNKNOWN";
}

inline OperationType string_to_operation_type(const std::string& s) {
   if (s == "MOVE") return OperationType::MOVE;
   if (s == "QUARANTINE") return OperationType::QUARANTINE;
   if (s == "UNDO_MOVE") return OperationType::UNDO_MOVE;
   if (s == "UNDO_QUARANTINE") return OperationType::UNDO_QUARANTINE;
   return OperationType::MOVE;
}

inline std::string operation_status_to_string(OperationStatus status) {
   switch (status) {
   case OperationStatus::SUCCESS: return "SUCCESS";
   case OperationStatus::FAILED: return "FAILED";
   case OperationStatus::PENDING: return "PENDING";
   case OperationStatus::ROLLED_BACK: return "ROLLED_BACK";
   }
   return "UNKNOWN";
}

inline OperationStatus string_to_operation_status(const std::string& s) {
   if (s == "SUCCESS") return OperationStatus::SUCCESS;
   if (s == "FAILED") return OperationStatus::FAILED;
   if (s == "PENDING") return OperationStatus::PENDING;
   if (s == "ROLLED_BACK") return OperationStatus::ROLLED_BACK;
   return OperationStatus::SUCCESS;
}
