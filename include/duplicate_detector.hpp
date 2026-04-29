#pragma once

#include "file_scanner.hpp"
#include <vector>
#include <map>
#include <optional>
#include <filesystem>
#include <functional>

namespace fs = std::filesystem;

struct DuplicateGroup {
  std::vector<fs::path> files;
  float similarity_score;
};

using ProgressCallback = std::function<void(size_t current, size_t total, const std::string& message)>;

class DuplicateDetector {
public:
  DuplicateDetector();

  std::vector<DuplicateGroup> find_duplicates(const std::vector<FileInfo>& files, ProgressCallback callback = nullptr);

  std::vector<DuplicateGroup> find_exact_duplicates(const std::vector<FileInfo>& files, ProgressCallback callback = nullptr);

private:
  std::optional<std::string> compute_file_hash(const fs::path& file);
};
