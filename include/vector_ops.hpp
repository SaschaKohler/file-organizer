#pragma once

#include <optional>
#include <vector>

class VectorOps {
public:
  // Computes cosine similarity between two vectors.
  // Returns std::nullopt if sizes mismatch or if either vector has zero norm.
  static std::optional<float> cosine_similarity(const std::vector<float>& a,
                                                const std::vector<float>& b);
};
