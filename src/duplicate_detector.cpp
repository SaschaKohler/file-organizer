#include "duplicate_detector.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>

DuplicateDetector::DuplicateDetector(EmbeddingEngine& engine, float threshold)
  : embedding_engine_(engine)
  , similarity_threshold_(threshold) {}

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

bool DuplicateDetector::are_similar(const std::vector<float>& emb1, const std::vector<float>& emb2) {
  auto similarity = VectorOps::cosine_similarity(emb1, emb2);
  if (!similarity.has_value()) {
    return false;
  }
  return *similarity >= similarity_threshold_;
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
  if (!embedding_engine_.is_enabled()) {
    return find_exact_duplicates(files, callback);
  }
  
  std::vector<std::pair<fs::path, std::vector<float>>> embeddings;
  
  size_t total = files.size();
  for (size_t i = 0; i < files.size(); ++i) {
    if (callback && i % 10 == 0) {
      callback(i, total, "Generating embeddings...");
    }
    
    auto emb = embedding_engine_.embed_file(files[i].path);
    if (emb.has_value()) {
      embeddings.push_back({files[i].path, *emb});
    }
  }
  
  if (embeddings.empty()) {
    return find_exact_duplicates(files, callback);
  }
  
  if (callback) {
    callback(0, embeddings.size(), "Comparing similarities...");
  }
  
  std::vector<DuplicateGroup> duplicates;
  std::vector<bool> processed(embeddings.size(), false);
  
  for (size_t i = 0; i < embeddings.size(); ++i) {
    if (processed[i]) continue;
    
    if (callback && i % 50 == 0) {
      callback(i, embeddings.size(), "Comparing similarities...");
    }
    
    DuplicateGroup group;
    group.files.push_back(embeddings[i].first);
    float max_similarity = 0.0f;
    
    for (size_t j = i + 1; j < embeddings.size(); ++j) {
      if (processed[j]) continue;
      
      if (are_similar(embeddings[i].second, embeddings[j].second)) {
        auto sim = VectorOps::cosine_similarity(embeddings[i].second, embeddings[j].second);
        if (sim.has_value()) {
          max_similarity = std::max(max_similarity, *sim);
          group.files.push_back(embeddings[j].first);
          processed[j] = true;
        }
      }
    }
    
    if (group.files.size() > 1) {
      group.similarity_score = max_similarity;
      duplicates.push_back(group);
      processed[i] = true;
    }
  }
  
  return duplicates;
}
