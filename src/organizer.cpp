#include "organizer.hpp"
#include "file_scanner.hpp"
#include "history_manager.hpp"
#include <chrono>
#include <cstddef>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

Organizer::Organizer(fs::path base_dir, bool dry_run)
    : base_dir_(std::move(base_dir)), dry_run_(dry_run) {}

void Organizer::add_rule(const std::string& category,
                         const std::string& target_dir, bool create_subdirs) {
  rules_[category] = OrganizeRule{.category = category,
                                  .target_dir = target_dir,
                                  .create_subdirs_by_date = create_subdirs};
}

std::optional<OrganizeRule>
Organizer::get_rule(const std::string& category) const {
  auto it = rules_.find(category);
  if (it != rules_.end()) {
    return it->second;
  }
  return std::nullopt;
}

std::string Organizer::get_date_subdir(const fs::path& file) const {
  auto ftime = fs::last_write_time(file);
  auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
      ftime - fs::file_time_type::clock::now() +
      std::chrono::system_clock::now());
  auto time = std::chrono::system_clock::to_time_t(sctp);

  std::tm tm = *std::localtime(&time);
  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m");
  return oss.str();
}

size_t Organizer::organize_all(const std::vector<FileInfo>& files) {
  size_t count = 0;

  for (const auto& file : files) {
    if (organize_file(file)) {
      ++count;
    }
  }

  return count;
}

bool Organizer::undo_last_operation() {
  if (move_history_.empty()) {
    return false;
  }
  
  auto op = move_history_.top();
  move_history_.pop();
  
  if (!op.was_successful) {
    return false;
  }
  
  try {
    if (fs::exists(op.destination)) {
      fs::create_directories(op.source.parent_path());
      fs::rename(op.destination, op.source);
      if (history_manager_) {
        history_manager_->record_undo_move(op.destination, op.source);
      }
      return true;
    }
  } catch (const fs::filesystem_error&) {
    return false;
  }
  
  return false;
}

fs::path Organizer::resolve_conflict(const fs::path& target) const {
  if (!fs::exists(target)) {
    return target;
  }

  auto stem = target.stem().string();
  auto ext = target.extension().string();
  auto parent = target.parent_path();

  for (int i = 1; i < 10000; ++i) {
    auto candidate = parent / (stem + " (" + std::to_string(i) + ")" + ext);
    if (!fs::exists(candidate)) {
      return candidate;
    }
  }

  return target;
}

void Organizer::record_move_to_history(const fs::path& source,
                                      const fs::path& dest,
                                      const std::string& category) {
   if (history_manager_) {
      std::error_code ec;
      int64_t fsize = 0;
      if (fs::exists(dest, ec)) {
         fsize = static_cast<int64_t>(fs::file_size(dest, ec));
      }
      history_manager_->record_move(source, dest, category, fsize);
   }
}

void Organizer::move_file(const fs::path& source, const fs::path& dest) {
  std::error_code ec;
  fs::rename(source, dest, ec);
  if (ec) {
    fs::copy_file(source, dest, fs::copy_options::overwrite_existing, ec);
    if (ec) {
      throw fs::filesystem_error("Failed to move file", source, dest, ec);
    }
    fs::remove(source, ec);
    if (ec) {
      fs::remove(dest);
      throw fs::filesystem_error("Failed to remove source after copy", source, ec);
    }
  }
}

void Organizer::clear_history() {
  while (!move_history_.empty()) {
    move_history_.pop();
  }
}
