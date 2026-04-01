#pragma once

#include "mime_detector.hpp"
#include <concepts>
#include <filesystem>
#include <memory>
#include <ranges>
#include <string>
#include <vector>

namespace fs = std::filesystem;

template <typename T>
concept PathLike = std::convertible_to<T, fs::path>;

struct FileInfo {
  fs::path path;
  std::string extension;
  uintmax_t size;
  fs::file_time_type modified_time;
  std::string category;

  FileInfo(const fs::path& p);
};

class FileScanner {
public:
  explicit FileScanner(fs::path root_dir, bool use_mime_detection = false, int max_depth = 0);

  std::vector<FileInfo> scan();

  void set_use_mime_detection(bool use_mime);
  [[nodiscard]] bool get_use_mime_detection() const {
    return use_mime_detection_;
  }
  
  void set_max_depth(int depth) { max_depth_ = depth; }
  [[nodiscard]] int get_max_depth() const { return max_depth_; }

  [[nodiscard]] auto get_files_by_category(const std::string& category) const {
    return files_ | std::views::filter([category](const FileInfo& f) {
             return f.category == category;
           });
  }

  template <PathLike P>
  void set_root(P&& path) {
    root_dir_ = std::forward<P>(path);
  }

  [[nodiscard]] const auto& files() const { return files_; }

private:
  fs::path root_dir_;
  std::vector<FileInfo> files_;
  bool use_mime_detection_;
  int max_depth_;
  std::unique_ptr<MimeDetector> mime_detector_;

  std::string categorize(const fs::path& path) const;
  std::string categorize_by_extension(const fs::path& path) const;
  void scan_recursive(const fs::path& dir, int current_depth);
};
