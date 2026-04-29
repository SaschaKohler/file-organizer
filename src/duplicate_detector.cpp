#include "duplicate_detector.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>

DuplicateDetector::DuplicateDetector() = default;

std::optional<std::string> DuplicateDetector::compute_file_hash(const fs::path& file) {
  if (!fs::exists(file) || !fs::is_regular_file(file)) {
    return std::nullopt;
  }

  std::ifstream ifs(file, std::ios::binary);
  if (!ifs) {
    return std::nullopt;
  }

  std::ostringstream oss;
  oss << fs::file_size(file) << "_";

  char buffer[1024];
  size_t hash = fs::file_size(file);

  while (ifs.read(buffer, sizeof(buffer)) || ifs.gcount() > 0) {
    for (std::streamsize i = 0; i < ifs.gcount(); ++i) {
      hash = hash * 31 + static_cast<unsigned char>(buffer[i]);
    }
  }

  oss << std::hex << hash;
  return oss.str();
}

std::vector<DuplicateGroup> DuplicateDetector::find_exact_duplicates(const std::vector<FileInfo>& files, ProgressCallback callback) {
  std::map<std::string, std::vector<fs::path>> hash_groups;

  size_t total = files.size();
  for (size_t i = 0; i < files.size(); ++i) {
    if (callback && i % 100 == 0) {
      callback(i, total, "Computing hashes...");
    }

    auto hash = compute_file_hash(files[i].path);
    if (hash.has_value()) {
      hash_groups[*hash].push_back(files[i].path);
    }
  }

  if (callback) {
    callback(total, total, "Grouping duplicates...");
  }

  std::vector<DuplicateGroup> duplicates;
  for (const auto& [hash, paths] : hash_groups) {
    if (paths.size() > 1) {
      DuplicateGroup group;
      group.files = paths;
      group.similarity_score = 1.0f;
      duplicates.push_back(group);
    }
  }

  return duplicates;
}

std::vector<DuplicateGroup> DuplicateDetector::find_duplicates(const std::vector<FileInfo>& files, ProgressCallback callback) {
  return find_exact_duplicates(files, callback);
}
