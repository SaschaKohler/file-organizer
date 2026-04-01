#include "quarantine.hpp"

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

// ─── helpers
// ──────────────────────────────────────────────────────────────────

std::string Quarantine::current_timestamp() {
   const auto now = std::chrono::system_clock::now();
   const std::time_t t = std::chrono::system_clock::to_time_t(now);
   std::tm tm{};
#ifdef _WIN32
   localtime_s(&tm, &t);
#else
   localtime_r(&t, &tm);
#endif
   std::ostringstream oss;
   // Use dashes instead of colons so the string is safe in file names.
   oss << std::put_time(&tm, "%Y-%m-%dT%H-%M-%S");
   return oss.str();
}

fs::path Quarantine::make_quarantine_path(const fs::path& original_path) const {
   const std::string stem =
       current_timestamp() + "_" + original_path.filename().string();
   return base_dir_ / stem;
}

// ─── manifest I/O
// ─────────────────────────────────────────────────────────────

void Quarantine::append_to_manifest(
    const QuarantineManifestEntry& entry) const {
   // Read existing manifest (may not exist yet).
   json arr = json::array();
   if (fs::exists(manifest_path_)) {
      std::ifstream in(manifest_path_);
      if (in.is_open()) {
         try {
            in >> arr;
         } catch (...) {
            arr = json::array();
         }
      }
   }

   // Append new entry.
   arr.push_back({
       {"timestamp", entry.timestamp},
       {"original_path", entry.original_path.string()},
       {"quarantine_path", entry.quarantine_path.string()},
       {"kept_path", entry.kept_path.string()},
       {"similarity", entry.similarity},
       {"method", entry.method},
   });

   std::ofstream out(manifest_path_);
   if (out.is_open()) {
      out << arr.dump(2);
   }
}

std::vector<QuarantineManifestEntry> Quarantine::load_manifest() const {
   if (!fs::exists(manifest_path_))
      return {};

   std::ifstream in(manifest_path_);
   if (!in.is_open())
      return {};

   json arr;
   try {
      in >> arr;
   } catch (...) {
      return {};
   }

   std::vector<QuarantineManifestEntry> result;
   for (const auto& obj : arr) {
      try {
         QuarantineManifestEntry e;
         e.timestamp = obj.at("timestamp").get<std::string>();
         e.original_path = obj.at("original_path").get<std::string>();
         e.quarantine_path = obj.at("quarantine_path").get<std::string>();
         e.kept_path = obj.at("kept_path").get<std::string>();
         e.similarity = obj.at("similarity").get<float>();
         e.method = obj.at("method").get<std::string>();
         result.push_back(std::move(e));
      } catch (...) {
         // Skip malformed entries.
      }
   }
   return result;
}

// ─── public API
// ───────────────────────────────────────────────────────────────

Quarantine::Quarantine(const fs::path& base_dir)
    : base_dir_(base_dir), manifest_path_(base_dir / "quarantine.json") {}

std::optional<fs::path>
Quarantine::quarantine_file(const fs::path& duplicate_path,
                            const fs::path& kept_path, float similarity,
                            const std::string& method) {

   if (!fs::exists(duplicate_path) || !fs::is_regular_file(duplicate_path)) {
      return std::nullopt;
   }

   // Ensure the quarantine directory exists.
   std::error_code ec;
   fs::create_directories(base_dir_, ec);
   if (ec)
      return std::nullopt;

   const fs::path dest = make_quarantine_path(duplicate_path);

   // Move the file (rename within same filesystem; copy+remove otherwise).
   fs::rename(duplicate_path, dest, ec);
   if (ec) {
      // Cross-device move: copy then remove.
      fs::copy_file(duplicate_path, dest, fs::copy_options::overwrite_existing,
                    ec);
      if (ec)
         return std::nullopt;
      fs::remove(duplicate_path, ec);
      if (ec) {
         // Couldn't remove original — roll back the copy.
         fs::remove(dest);
         return std::nullopt;
      }
   }

   // Push to in-memory undo stack.
   undo_stack_.push_back({duplicate_path, dest});

   // Persist to manifest.
   QuarantineManifestEntry entry;
   entry.timestamp = current_timestamp();
   entry.original_path = duplicate_path;
   entry.quarantine_path = dest;
   entry.kept_path = kept_path;
   entry.similarity = similarity;
   entry.method = method;
   append_to_manifest(entry);

   return dest;
}

bool Quarantine::undo_last() {
   if (undo_stack_.empty())
      return false;

   const auto [original, quarantine] = undo_stack_.back();

   if (!fs::exists(quarantine)) {
      undo_stack_.pop_back();
      return false;
   }

   // Recreate parent directory if it was removed.
   std::error_code ec;
   if (original.has_parent_path()) {
      fs::create_directories(original.parent_path(), ec);
      if (ec)
         return false;
   }

   fs::rename(quarantine, original, ec);
   if (ec) {
      fs::copy_file(quarantine, original, fs::copy_options::overwrite_existing,
                    ec);
      if (ec)
         return false;
      fs::remove(quarantine);
   }

   undo_stack_.pop_back();
   return true;
}

size_t Quarantine::purge(int older_than_days) {
   if (!fs::exists(base_dir_))
      return 0;

   // Stay in file_clock to avoid cross-clock conversion (clock_cast is not
   // reliably available on all platforms).
   const auto now_ft = fs::file_time_type::clock::now();
   size_t deleted = 0;

   for (const auto& de : fs::directory_iterator(base_dir_)) {
      if (!de.is_regular_file())
         continue;
      if (de.path() == manifest_path_)
         continue; // Keep the manifest.

      bool should_delete = false;
      if (older_than_days <= 0) {
         should_delete = true;
      } else {
         std::error_code ec;
         const auto mtime = de.last_write_time(ec);
         if (!ec) {
            const auto age_h =
                std::chrono::duration_cast<std::chrono::hours>(now_ft - mtime)
                    .count();
            should_delete =
                age_h >= static_cast<long long>(older_than_days) * 24;
         }
      }

      if (should_delete) {
         std::error_code ec;
         fs::remove(de.path(), ec);
         if (!ec)
            ++deleted;
      }
   }

   // Remove stale entries from the in-memory undo stack.
   undo_stack_.erase(std::remove_if(undo_stack_.begin(), undo_stack_.end(),
                                    [](const UndoEntry& e) {
                                       return !fs::exists(e.quarantine_path);
                                    }),
                     undo_stack_.end());

   return deleted;
}

size_t Quarantine::undo_stack_size() const { return undo_stack_.size(); }
