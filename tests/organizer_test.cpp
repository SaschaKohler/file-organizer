#include "organizer.hpp"
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

class OrganizerTest : public ::testing::Test {
protected:
  fs::path test_dir_;
  fs::path organize_dir_;

  void SetUp() override {
    auto pid = std::to_string(getpid());
    test_dir_ = fs::temp_directory_path() / ("organizer_test_src_" + pid);
    organize_dir_ = fs::temp_directory_path() / ("organizer_test_dst_" + pid);
    fs::remove_all(test_dir_);
    fs::remove_all(organize_dir_);
    fs::create_directories(test_dir_);
    fs::create_directories(organize_dir_);
  }

  void TearDown() override {
    if (fs::exists(test_dir_)) {
      fs::remove_all(test_dir_);
    }
    if (fs::exists(organize_dir_)) {
      fs::remove_all(organize_dir_);
    }
  }

  FileInfo create_test_file(const std::string& filename, const std::string& category) {
    std::ofstream file(test_dir_ / filename);
    file << "test content";
    file.close();
    
    FileInfo info(test_dir_ / filename);
    info.category = category;
    return info;
  }
};

TEST_F(OrganizerTest, DryRunDoesNotMoveFiles) {
  auto file = create_test_file("test.txt", "documents");
  
  Organizer organizer(organize_dir_, true);
  organizer.add_rule("documents", "Documents");
  
  auto result = organizer.organize_file(file);
  
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(fs::exists(file.path));
  EXPECT_FALSE(fs::exists(*result));
}

TEST_F(OrganizerTest, LiveModeMovesFiles) {
  auto file = create_test_file("test.txt", "documents");
  
  Organizer organizer(organize_dir_, false);
  organizer.add_rule("documents", "Documents");
  
  auto result = organizer.organize_file(file);
  
  ASSERT_TRUE(result.has_value());
  EXPECT_FALSE(fs::exists(file.path));
  EXPECT_TRUE(fs::exists(*result));
}

TEST_F(OrganizerTest, CreatesTargetDirectory) {
  auto file = create_test_file("test.txt", "documents");
  
  Organizer organizer(organize_dir_, false);
  organizer.add_rule("documents", "Documents");
  
  auto result = organizer.organize_file(file);
  
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(fs::exists(organize_dir_ / "Documents"));
}

TEST_F(OrganizerTest, NoRuleReturnsNullopt) {
  auto file = create_test_file("test.txt", "unknown");
  
  Organizer organizer(organize_dir_, false);
  
  auto result = organizer.organize_file(file);
  
  EXPECT_FALSE(result.has_value());
  EXPECT_TRUE(fs::exists(file.path));
}

TEST_F(OrganizerTest, MultipleRules) {
  auto doc = create_test_file("doc.pdf", "documents");
  auto img = create_test_file("photo.jpg", "images");
  
  Organizer organizer(organize_dir_, false);
  organizer.add_rule("documents", "Documents");
  organizer.add_rule("images", "Images");
  
  auto doc_result = organizer.organize_file(doc);
  auto img_result = organizer.organize_file(img);
  
  ASSERT_TRUE(doc_result.has_value());
  ASSERT_TRUE(img_result.has_value());
  
  EXPECT_TRUE(fs::exists(organize_dir_ / "Documents" / "doc.pdf"));
  EXPECT_TRUE(fs::exists(organize_dir_ / "Images" / "photo.jpg"));
}

TEST_F(OrganizerTest, NestedTargetDirectory) {
  auto file = create_test_file("data.xlsx", "spreadsheets");
  
  Organizer organizer(organize_dir_, false);
  organizer.add_rule("spreadsheets", "Documents/Spreadsheets");
  
  auto result = organizer.organize_file(file);
  
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(fs::exists(organize_dir_ / "Documents" / "Spreadsheets" / "data.xlsx"));
}

TEST_F(OrganizerTest, DateSubdirectories) {
  auto file = create_test_file("report.pdf", "documents");
  
  Organizer organizer(organize_dir_, false);
  organizer.add_rule("documents", "Documents", true);
  
  auto result = organizer.organize_file(file);
  
  ASSERT_TRUE(result.has_value());
  
  auto parent = result->parent_path();
  EXPECT_TRUE(parent.filename().string().find("2026") != std::string::npos ||
              parent.filename().string().find("2025") != std::string::npos);
}

TEST_F(OrganizerTest, OrganizeAll) {
  create_test_file("doc1.pdf", "documents");
  create_test_file("doc2.txt", "documents");
  create_test_file("photo.jpg", "images");
  
  std::vector<FileInfo> files;
  for (const auto& entry : fs::directory_iterator(test_dir_)) {
    FileInfo info(entry.path());
    if (entry.path().extension() == ".pdf" || entry.path().extension() == ".txt") {
      info.category = "documents";
    } else {
      info.category = "images";
    }
    files.push_back(info);
  }
  
  Organizer organizer(organize_dir_, false);
  organizer.add_rule("documents", "Documents");
  organizer.add_rule("images", "Images");
  
  size_t count = organizer.organize_all(files);
  
  EXPECT_EQ(count, 3);
  EXPECT_TRUE(fs::exists(organize_dir_ / "Documents"));
  EXPECT_TRUE(fs::exists(organize_dir_ / "Images"));
}

TEST_F(OrganizerTest, ToggleDryRun) {
  Organizer organizer(organize_dir_, true);
  
  EXPECT_TRUE(organizer.is_dry_run());
  
  organizer.set_dry_run(false);
  EXPECT_FALSE(organizer.is_dry_run());
  
  organizer.set_dry_run(true);
  EXPECT_TRUE(organizer.is_dry_run());
}

TEST_F(OrganizerTest, PreservesFilename) {
  auto file = create_test_file("my_important_file.txt", "documents");
  
  Organizer organizer(organize_dir_, false);
  organizer.add_rule("documents", "Documents");
  
  auto result = organizer.organize_file(file);
  
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->filename(), "my_important_file.txt");
}

TEST_F(OrganizerTest, HandlesSpecialCharactersInFilename) {
  auto file = create_test_file("file with spaces & special.txt", "documents");
  
  Organizer organizer(organize_dir_, false);
  organizer.add_rule("documents", "Documents");
  
  auto result = organizer.organize_file(file);
  
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(fs::exists(*result));
}

TEST_F(OrganizerTest, OrganizableConceptWorks) {
  struct CustomFile {
    fs::path path;
    std::string category;
  };
  
  CustomFile custom{test_dir_ / "custom.txt", "documents"};
  std::ofstream(custom.path) << "test";
  
  Organizer organizer(organize_dir_, false);
  organizer.add_rule("documents", "Documents");
  
  auto result = organizer.organize_file(custom);
  
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(fs::exists(*result));
}
