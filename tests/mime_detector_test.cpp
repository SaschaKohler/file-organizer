#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include "mime_detector.hpp"

namespace fs = std::filesystem;

class MimeDetectorTest : public ::testing::Test {
protected:
  void SetUp() override {
    test_dir_ = fs::temp_directory_path() /
                ("mime_detector_test_" + std::to_string(getpid()));
    fs::remove_all(test_dir_);
    fs::create_directories(test_dir_);
  }

  void TearDown() override {
    if (fs::exists(test_dir_)) {
      fs::remove_all(test_dir_);
    }
  }

  fs::path create_test_file(const std::string& name, const std::string& content) {
    auto path = test_dir_ / name;
    std::ofstream file(path);
    file << content;
    file.close();
    return path;
  }

  fs::path test_dir_;
};

TEST_F(MimeDetectorTest, DetectTextPlain) {
  auto path = create_test_file("test.txt", "Hello, World!");
  MimeDetector detector;
  
  auto mime = detector.detect(path);
  ASSERT_TRUE(mime.has_value());
  EXPECT_EQ(*mime, "text/plain");
}

TEST_F(MimeDetectorTest, DetectImagePNG) {
  auto path = test_dir_ / "test.png";
  std::ofstream file(path, std::ios::binary);
  unsigned char png_data[] = {
    0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A,
    0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52,
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
    0x08, 0x02, 0x00, 0x00, 0x00, 0x90, 0x77, 0x53,
    0xDE, 0x00, 0x00, 0x00, 0x0C, 0x49, 0x44, 0x41,
    0x54, 0x08, 0xD7, 0x63, 0xF8, 0xCF, 0xC0, 0x00,
    0x00, 0x03, 0x01, 0x01, 0x00, 0x18, 0xDD, 0x8D,
    0xB4, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E,
    0x44, 0xAE, 0x42, 0x60, 0x82
  };
  file.write(reinterpret_cast<char*>(png_data), sizeof(png_data));
  file.close();
  
  MimeDetector detector;
  auto mime = detector.detect(path);
  ASSERT_TRUE(mime.has_value());
  EXPECT_TRUE(mime->find("image") != std::string::npos || mime->find("png") != std::string::npos);
}

TEST_F(MimeDetectorTest, DetectNonExistentFile) {
  MimeDetector detector;
  auto mime = detector.detect(test_dir_ / "nonexistent.txt");
  EXPECT_FALSE(mime.has_value());
}

TEST_F(MimeDetectorTest, DetectEmptyFile) {
  auto path = create_test_file("empty.txt", "");
  MimeDetector detector;
  
  auto mime = detector.detect(path);
  ASSERT_TRUE(mime.has_value());
  EXPECT_FALSE(mime->empty());
}

TEST_F(MimeDetectorTest, DetectJSONFile) {
  auto path = create_test_file("test.json", R"({"key": "value"})");
  MimeDetector detector;
  
  auto mime = detector.detect(path);
  ASSERT_TRUE(mime.has_value());
  EXPECT_TRUE(mime->find("text") != std::string::npos || 
              mime->find("json") != std::string::npos ||
              mime->find("application") != std::string::npos);
}

TEST_F(MimeDetectorTest, CategoryFromMimeImage) {
  MimeDetector detector;
  EXPECT_EQ(detector.category_from_mime("image/png"), "images");
  EXPECT_EQ(detector.category_from_mime("image/jpeg"), "images");
  EXPECT_EQ(detector.category_from_mime("image/gif"), "images");
}

TEST_F(MimeDetectorTest, CategoryFromMimeVideo) {
  MimeDetector detector;
  EXPECT_EQ(detector.category_from_mime("video/mp4"), "videos");
  EXPECT_EQ(detector.category_from_mime("video/mpeg"), "videos");
}

TEST_F(MimeDetectorTest, CategoryFromMimeAudio) {
  MimeDetector detector;
  EXPECT_EQ(detector.category_from_mime("audio/mpeg"), "audio");
  EXPECT_EQ(detector.category_from_mime("audio/wav"), "audio");
}

TEST_F(MimeDetectorTest, CategoryFromMimeDocument) {
  MimeDetector detector;
  EXPECT_EQ(detector.category_from_mime("application/pdf"), "documents");
  EXPECT_EQ(detector.category_from_mime("application/msword"), "documents");
}

TEST_F(MimeDetectorTest, CategoryFromMimeArchive) {
  MimeDetector detector;
  EXPECT_EQ(detector.category_from_mime("application/zip"), "archives");
  EXPECT_EQ(detector.category_from_mime("application/x-tar"), "archives");
}

TEST_F(MimeDetectorTest, CategoryFromMimeUnknown) {
  MimeDetector detector;
  EXPECT_EQ(detector.category_from_mime("application/octet-stream"), "other");
  EXPECT_EQ(detector.category_from_mime("unknown/type"), "other");
}

TEST_F(MimeDetectorTest, CategoryFromMimeText) {
  MimeDetector detector;
  EXPECT_EQ(detector.category_from_mime("text/plain"), "documents");
  EXPECT_EQ(detector.category_from_mime("text/html"), "documents");
}
