#pragma once

#include "file_scanner.hpp"
#include "embedding_engine.hpp"
#include "vector_ops.hpp"
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
  DuplicateDetector(EmbeddingEngine& engine, float threshold = 0.95f);
  
  std::vector<DuplicateGroup> find_duplicates(const std::vector<FileInfo>& files, ProgressCallback callback = nullptr);
  
  std::vector<DuplicateGroup> find_exact_duplicates(const std::vector<FileInfo>& files, ProgressCallback callback = nullptr);
  
  void set_similarity_threshold(float threshold) { similarity_threshold_ = threshold; }
  [[nodiscard]] float get_similarity_threshold() const { return similarity_threshold_; }
  
private:
  EmbeddingEngine& embedding_engine_;
  float similarity_threshold_;
  
  std::optional<std::string> compute_file_hash(const fs::path& file);
  bool are_similar(const std::vector<float>& emb1, const std::vector<float>& emb2);
};
