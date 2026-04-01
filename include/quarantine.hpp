#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// Records one entry in the on-disk manifest.
struct QuarantineManifestEntry {
   std::string timestamp;    // ISO-8601, e.g. "2026-04-01T13:24:46"
   fs::path original_path;   // Where the file lived before quarantine
   fs::path quarantine_path; // Where it lives now
   fs::path kept_path;       // The copy that was kept (the "original")
   float similarity;         // 0.0–1.0 similarity score
   std::string method;       // "hash" | "simhash"
};

// Manages a quarantine directory: moves duplicate files there safely,
// keeps a JSON manifest for auditing, and provides undo / purge.
//
// Directory layout:
//   <base_dir>/
//     quarantine.json                    ← audit manifest
//     2026-04-01T13-24-46_copy.pdf       ← quarantined file
//     2026-04-01T13-25-12_photo.jpg
//
// The timestamp prefix guarantees uniqueness even for files with the
// same name.
class Quarantine {
 public:
   explicit Quarantine(const fs::path& base_dir);

   // Move `duplicate_path` into the quarantine directory.
   // `kept_path` is recorded as the file that was kept (informational).
   // Returns the new quarantine path on success, std::nullopt on failure
   // (file doesn't exist, can't move, …).
   std::optional<fs::path> quarantine_file(const fs::path& duplicate_path,
                                           const fs::path& kept_path,
                                           float similarity,
                                           const std::string& method = "hash");

   // Undo the most recent quarantine operation: move the file back to its
   // original location.  Returns false if nothing can be undone or the
   // quarantine file is missing.
   bool undo_last();

   // Permanently delete quarantined files that were quarantined more than
   // `older_than_days` days ago.  Pass 0 to delete all quarantined files.
   // Returns the number of files deleted.
   size_t purge(int older_than_days = 0);

   // Number of operations remaining in the in-memory undo stack.
   [[nodiscard]] size_t undo_stack_size() const;

   [[nodiscard]] const fs::path& base_dir() const { return base_dir_; }

   // Load and return all entries from the on-disk manifest.
   [[nodiscard]] std::vector<QuarantineManifestEntry> load_manifest() const;

 private:
   struct UndoEntry {
      fs::path original_path;
      fs::path quarantine_path;
   };

   fs::path base_dir_;
   fs::path manifest_path_;
   std::vector<UndoEntry> undo_stack_;

   // Compute a unique quarantine path for `original_path`:
   // <base_dir>/<timestamp>_<filename>
   [[nodiscard]] fs::path
   make_quarantine_path(const fs::path& original_path) const;

   // Append one entry to the JSON manifest (read-modify-write).
   void append_to_manifest(const QuarantineManifestEntry& entry) const;

   // Return the current local time as an ISO-8601 string safe for filenames
   // (colons replaced with dashes): "2026-04-01T13-24-46"
   [[nodiscard]] static std::string current_timestamp();
};
