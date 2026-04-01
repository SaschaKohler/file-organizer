#include <gtest/gtest.h>
#include "vector_ops.hpp"

TEST(VectorOpsTest, SameVectorsHaveSimilarityOne) {
  std::vector<float> a{1.0f, 2.0f, 3.0f};
  auto sim = VectorOps::cosine_similarity(a, a);
  ASSERT_TRUE(sim.has_value());
  EXPECT_FLOAT_EQ(*sim, 1.0f);
}

TEST(VectorOpsTest, OrthogonalVectorsHaveSimilarityZero) {
  std::vector<float> a{1.0f, 0.0f};
  std::vector<float> b{0.0f, 1.0f};
  auto sim = VectorOps::cosine_similarity(a, b);
  ASSERT_TRUE(sim.has_value());
  EXPECT_FLOAT_EQ(*sim, 0.0f);
}

TEST(VectorOpsTest, SizeMismatchReturnsNullopt) {
  std::vector<float> a{1.0f, 2.0f};
  std::vector<float> b{1.0f, 2.0f, 3.0f};
  auto sim = VectorOps::cosine_similarity(a, b);
  EXPECT_FALSE(sim.has_value());
}

TEST(VectorOpsTest, ZeroVectorReturnsNullopt) {
  std::vector<float> a{0.0f, 0.0f, 0.0f};
  std::vector<float> b{1.0f, 2.0f, 3.0f};
  auto sim = VectorOps::cosine_similarity(a, b);
  EXPECT_FALSE(sim.has_value());
}

TEST(VectorOpsTest, BasicSimilarity) {
  std::vector<float> a{1.0f, 2.0f, 3.0f};
  std::vector<float> b{1.0f, 2.0f, 4.0f};
  auto sim = VectorOps::cosine_similarity(a, b);
  ASSERT_TRUE(sim.has_value());
  EXPECT_NEAR(*sim, 0.9915f, 2e-3f);
}
