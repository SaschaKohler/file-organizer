#pragma once

#include "file_scanner.hpp"
#include <concepts>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>
#include <stack>

template <typename T>
concept Organizable = requires(T t) {
  { t.path } -> std::convertible_to<fs::path>;
  { t.category } -> std::convertible_to<std::string>;
};

struct OrganizeRule {
  std::string category;
  std::string target_dir;
  bool create_subdirs_by_date = false;
};

struct MoveOperation {
  fs::path source;
  fs::path destination;
  bool was_successful = false;
};

class Organizer {
public:
  Organizer(fs::path base_dir, bool dry_run = false);

  void add_rule(const std::string& category, const std::string& target_dir,
                bool create_subdirs = false);

  template <Organizable T>
  std::optional<fs::path> organize_file(const T& file) {
    auto rule = get_rule(file.category);
    if (!rule) {
      return std::nullopt;
    }

    fs::path target = base_dir_ / rule->target_dir;

    if (rule->create_subdirs_by_date) {
      target /= get_date_subdir(file.path);
    }

    target /= file.path.filename();
    target = resolve_conflict(target);

    if (!dry_run_) {
      fs::create_directories(target.parent_path());
      move_file(file.path, target);
      
      MoveOperation op;
      op.source = file.path;
      op.destination = target;
      op.was_successful = true;
      move_history_.push(op);
    }

    return target;
  }

  size_t organize_all(const std::vector<FileInfo>& files);

  void set_dry_run(bool dry_run) { dry_run_ = dry_run; }
  [[nodiscard]] bool is_dry_run() const { return dry_run_; }
  
  bool undo_last_operation();
  void clear_history();
  [[nodiscard]] size_t history_size() const { return move_history_.size(); }
  [[nodiscard]] const fs::path& get_base_dir() const { return base_dir_; }
  void set_base_dir(const fs::path& base_dir) { base_dir_ = base_dir; }

private:
  fs::path base_dir_;
  bool dry_run_;
  std::map<std::string, OrganizeRule> rules_;
  std::stack<MoveOperation> move_history_;

  [[nodiscard]] std::optional<OrganizeRule>
  get_rule(const std::string& category) const;
  [[nodiscard]] std::string get_date_subdir(const fs::path& file) const;
  [[nodiscard]] fs::path resolve_conflict(const fs::path& target) const;
  void move_file(const fs::path& source, const fs::path& dest);
};
