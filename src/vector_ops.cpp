#include "vector_ops.hpp"

#include <Eigen/Dense>
#include <cmath>

std::optional<float> VectorOps::cosine_similarity(const std::vector<float>& a,
                                                  const std::vector<float>& b) {
   if (a.size() != b.size() || a.empty()) {
      return std::nullopt;
   }

   Eigen::Map<const Eigen::VectorXf> va(a.data(),
                                        static_cast<Eigen::Index>(a.size()));
   Eigen::Map<const Eigen::VectorXf> vb(b.data(),
                                        static_cast<Eigen::Index>(b.size()));

   const float norm_a = va.norm();
   const float norm_b = vb.norm();
   if (norm_a == 0.0F || norm_b == 0.0F) {
      return std::nullopt;
   }

   const float dot = va.dot(vb);
   const float denom = norm_a * norm_b;
   return dot / denom;
}
