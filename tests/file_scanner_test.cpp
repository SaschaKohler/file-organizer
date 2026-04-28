#include "file_scanner.hpp"
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

namespace fs = std::filesystem;

class FileScannerTest : public ::testing::Test {
protected:
  fs::path test_dir_;

  void SetUp() override {
    test_dir_ = fs::temp_directory_path() /
                ("file_organizer_test_" + std::to_string(getpid()));
    fs::remove_all(test_dir_);
    fs::create_directories(test_dir_);
  }

  void TearDown() override {
    if (fs::exists(test_dir_)) {
      fs::remove_all(test_dir_);
    }
  }

  void create_test_file(const std::string& filename, const std::string& content = "test") {
    std::ofstream file(test_dir_ / filename);
    file << content;
  }
};

TEST_F(FileScannerTest, ScanEmptyDirectory) {
  FileScanner scanner(test_dir_);
  auto files = scanner.scan();
  EXPECT_TRUE(files.empty());
}

TEST_F(FileScannerTest, ScanSingleFile) {
  create_test_file("test.txt");
  
  FileScanner scanner(test_dir_);
  auto files = scanner.scan();
  
  ASSERT_EQ(files.size(), 1);
  EXPECT_EQ(files[0].path.filename(), "test.txt");
  EXPECT_EQ(files[0].extension, ".txt");
  EXPECT_EQ(files[0].category, "documents");
}

TEST_F(FileScannerTest, CategorizeImages) {
  create_test_file("photo.jpg");
  create_test_file("image.png");
  create_test_file("graphic.svg");
  
  FileScanner scanner(test_dir_);
  auto files = scanner.scan();
  
  ASSERT_EQ(files.size(), 3);
  for (const auto& file : files) {
    EXPECT_EQ(file.category, "images");
  }
}

TEST_F(FileScannerTest, CategorizeVideos) {
  create_test_file("movie.mp4");
  create_test_file("clip.avi");
  
  FileScanner scanner(test_dir_);
  auto files = scanner.scan();
  
  ASSERT_EQ(files.size(), 2);
  for (const auto& file : files) {
    EXPECT_EQ(file.category, "videos");
  }
}

TEST_F(FileScannerTest, CategorizeAudio) {
  create_test_file("song.mp3");
  create_test_file("track.wav");
  
  FileScanner scanner(test_dir_);
  auto files = scanner.scan();
  
  ASSERT_EQ(files.size(), 2);
  for (const auto& file : files) {
    EXPECT_EQ(file.category, "audio");
  }
}

TEST_F(FileScannerTest, CategorizeDocuments) {
  create_test_file("report.pdf");
  create_test_file("notes.txt");
  create_test_file("readme.md");
  
  FileScanner scanner(test_dir_);
  auto files = scanner.scan();
  
  ASSERT_EQ(files.size(), 3);
  for (const auto& file : files) {
    EXPECT_EQ(file.category, "documents");
  }
}

TEST_F(FileScannerTest, CategorizeExtendedImages) {
  create_test_file("photo.heic");
  create_test_file("raw.cr2");
  create_test_file("design.psd");
  create_test_file("vector.eps");
  
  FileScanner scanner(test_dir_);
  auto files = scanner.scan();
  
  ASSERT_EQ(files.size(), 4);
  for (const auto& file : files) {
    EXPECT_EQ(file.category, "images");
  }
}

TEST_F(FileScannerTest, CategorizeExtendedVideos) {
  create_test_file("video.webm");
  create_test_file("movie.m4v");
  create_test_file("clip.3gp");
  create_test_file("recording.mts");
  
  FileScanner scanner(test_dir_);
  auto files = scanner.scan();
  
  ASSERT_EQ(files.size(), 4);
  for (const auto& file : files) {
    EXPECT_EQ(file.category, "videos");
  }
}

TEST_F(FileScannerTest, CategorizeExtendedAudio) {
  create_test_file("song.opus");
  create_test_file("track.wma");
  create_test_file("music.aiff");
  create_test_file("melody.mid");
  
  FileScanner scanner(test_dir_);
  auto files = scanner.scan();
  
  ASSERT_EQ(files.size(), 4);
  for (const auto& file : files) {
    EXPECT_EQ(file.category, "audio");
  }
}

TEST_F(FileScannerTest, CategorizeEbooks) {
  create_test_file("book.epub");
  create_test_file("novel.mobi");
  create_test_file("manual.azw3");
  
  FileScanner scanner(test_dir_);
  auto files = scanner.scan();
  
  ASSERT_EQ(files.size(), 3);
  for (const auto& file : files) {
    EXPECT_EQ(file.category, "documents");
  }
}

TEST_F(FileScannerTest, CategorizeExtendedCode) {
  create_test_file("script.jsx");
  create_test_file("component.tsx");
  create_test_file("config.yaml");
  create_test_file("styles.scss");
  create_test_file("app.vue");
  create_test_file("widget.svelte");
  
  FileScanner scanner(test_dir_);
  auto files = scanner.scan();
  
  ASSERT_EQ(files.size(), 6);
  for (const auto& file : files) {
    EXPECT_EQ(file.category, "code");
  }
}

TEST_F(FileScannerTest, CategorizeExtendedArchives) {
  create_test_file("backup.tgz");
  create_test_file("data.xz");
  create_test_file("disk.iso");
  create_test_file("image.dmg");
  
  FileScanner scanner(test_dir_);
  auto files = scanner.scan();
  
  ASSERT_EQ(files.size(), 4);
  for (const auto& file : files) {
    EXPECT_EQ(file.category, "archives");
  }
}

TEST_F(FileScannerTest, CategorizeExtendedInstallers) {
  create_test_file("app.apk");
  create_test_file("package.deb");
  create_test_file("software.rpm");
  create_test_file("program.appimage");
  
  FileScanner scanner(test_dir_);
  auto files = scanner.scan();
  
  ASSERT_EQ(files.size(), 4);
  for (const auto& file : files) {
    EXPECT_EQ(file.category, "installers");
  }
}

TEST_F(FileScannerTest, CategorizeSpreadsheets) {
  create_test_file("data.numbers");
  create_test_file("table.tsv");
  create_test_file("report.xlsm");
  
  FileScanner scanner(test_dir_);
  auto files = scanner.scan();
  
  ASSERT_EQ(files.size(), 3);
  for (const auto& file : files) {
    EXPECT_EQ(file.category, "spreadsheets");
  }
}

TEST_F(FileScannerTest, CategorizePresentations) {
  create_test_file("slides.key");
  create_test_file("deck.ppsx");
  
  FileScanner scanner(test_dir_);
  auto files = scanner.scan();
  
  ASSERT_EQ(files.size(), 2);
  for (const auto& file : files) {
    EXPECT_EQ(file.category, "presentations");
  }
}

TEST_F(FileScannerTest, CategorizeCode) {
  create_test_file("main.cpp");
  create_test_file("script.py");
  create_test_file("app.js");
  
  FileScanner scanner(test_dir_);
  auto files = scanner.scan();
  
  ASSERT_EQ(files.size(), 3);
  for (const auto& file : files) {
    EXPECT_EQ(file.category, "code");
  }
}

TEST_F(FileScannerTest, CategorizeArchives) {
  create_test_file("backup.zip");
  create_test_file("data.tar");
  create_test_file("archive.7z");
  
  FileScanner scanner(test_dir_);
  auto files = scanner.scan();
  
  ASSERT_EQ(files.size(), 3);
  for (const auto& file : files) {
    EXPECT_EQ(file.category, "archives");
  }
}

TEST_F(FileScannerTest, CategorizeUnknownAsOther) {
  create_test_file("unknown.xyz");
  
  FileScanner scanner(test_dir_);
  auto files = scanner.scan();
  
  ASSERT_EQ(files.size(), 1);
  EXPECT_EQ(files[0].category, "other");
}

TEST_F(FileScannerTest, MixedCategories) {
  create_test_file("photo.jpg");
  create_test_file("report.pdf");
  create_test_file("song.mp3");
  create_test_file("script.py");
  
  FileScanner scanner(test_dir_);
  auto files = scanner.scan();
  
  ASSERT_EQ(files.size(), 4);
  
  std::map<std::string, int> category_counts;
  for (const auto& file : files) {
    category_counts[file.category]++;
  }
  
  EXPECT_EQ(category_counts["images"], 1);
  EXPECT_EQ(category_counts["documents"], 1);
  EXPECT_EQ(category_counts["audio"], 1);
  EXPECT_EQ(category_counts["code"], 1);
}

TEST_F(FileScannerTest, FileSize) {
  std::string content(1024, 'x');
  create_test_file("large.txt", content);
  
  FileScanner scanner(test_dir_);
  auto files = scanner.scan();
  
  ASSERT_EQ(files.size(), 1);
  EXPECT_GE(files[0].size, 1024);
}

TEST_F(FileScannerTest, SortedByModifiedTime) {
  create_test_file("first.txt");
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  create_test_file("second.txt");
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  create_test_file("third.txt");
  
  FileScanner scanner(test_dir_);
  auto files = scanner.scan();
  
  ASSERT_EQ(files.size(), 3);
  EXPECT_GE(files[0].modified_time, files[1].modified_time);
  EXPECT_GE(files[1].modified_time, files[2].modified_time);
}

TEST_F(FileScannerTest, GetFilesByCategory) {
  create_test_file("photo1.jpg");
  create_test_file("photo2.png");
  create_test_file("doc.pdf");
  
  FileScanner scanner(test_dir_);
  scanner.scan();
  
  auto images = scanner.get_files_by_category("images");
  int image_count = 0;
  for (const auto& file : images) {
    image_count++;
    EXPECT_EQ(file.category, "images");
  }
  EXPECT_EQ(image_count, 2);
}

TEST_F(FileScannerTest, SetRoot) {
  FileScanner scanner(test_dir_);
  
  fs::path new_dir = fs::temp_directory_path() /
                     ("file_organizer_test2_" + std::to_string(getpid()));
  fs::create_directories(new_dir);
  
  scanner.set_root(new_dir);
  auto files = scanner.scan();
  EXPECT_TRUE(files.empty());
  
  fs::remove_all(new_dir);
}

TEST_F(FileScannerTest, NonExistentDirectory) {
  fs::path non_existent = test_dir_ / "does_not_exist";
  
  FileScanner scanner(non_existent);
  auto files = scanner.scan();
  
  EXPECT_TRUE(files.empty());
}

TEST_F(FileScannerTest, CaseInsensitiveExtensions) {
  create_test_file("PHOTO.JPG");
  create_test_file("Image.PNG");
  
  FileScanner scanner(test_dir_);
  auto files = scanner.scan();
  
  ASSERT_EQ(files.size(), 2);
  for (const auto& file : files) {
    EXPECT_EQ(file.category, "images");
  }
}

TEST_F(FileScannerTest, MimeDetectionDisabledByDefault) {
  FileScanner scanner(test_dir_);
  EXPECT_FALSE(scanner.get_use_mime_detection());
}

TEST_F(FileScannerTest, MimeDetectionCanBeEnabled) {
  FileScanner scanner(test_dir_, true);
  EXPECT_TRUE(scanner.get_use_mime_detection());
}

TEST_F(FileScannerTest, MimeDetectionFallbackChain) {
  create_test_file("test.txt", "Hello, World!");
  
  FileScanner scanner_no_mime(test_dir_, false);
  auto files_no_mime = scanner_no_mime.scan();
  ASSERT_EQ(files_no_mime.size(), 1);
  EXPECT_EQ(files_no_mime[0].category, "documents");
  
  FileScanner scanner_with_mime(test_dir_, true);
  auto files_with_mime = scanner_with_mime.scan();
  ASSERT_EQ(files_with_mime.size(), 1);
  EXPECT_EQ(files_with_mime[0].category, "documents");
}

TEST_F(FileScannerTest, MimeDetectionForUnknownExtension) {
  auto path = test_dir_ / "test.unknown";
  std::ofstream file(path);
  file << "Plain text content";
  file.close();
  
  FileScanner scanner_no_mime(test_dir_, false);
  auto files_no_mime = scanner_no_mime.scan();
  ASSERT_EQ(files_no_mime.size(), 1);
  EXPECT_EQ(files_no_mime[0].category, "other");
  
  FileScanner scanner_with_mime(test_dir_, true);
  auto files_with_mime = scanner_with_mime.scan();
  ASSERT_EQ(files_with_mime.size(), 1);
  EXPECT_EQ(files_with_mime[0].category, "documents");
}

TEST_F(FileScannerTest, ToggleMimeDetection) {
  FileScanner scanner(test_dir_);
  EXPECT_FALSE(scanner.get_use_mime_detection());
  
  scanner.set_use_mime_detection(true);
  EXPECT_TRUE(scanner.get_use_mime_detection());
  
  scanner.set_use_mime_detection(false);
  EXPECT_FALSE(scanner.get_use_mime_detection());
}

TEST_F(FileScannerTest, HandlesSymlinks) {
  create_test_file("original.txt");
  fs::path symlink_path = test_dir_ / "link.txt";
  
  std::error_code ec;
  fs::create_symlink(test_dir_ / "original.txt", symlink_path, ec);
  
  if (!ec) {
    FileScanner scanner(test_dir_);
    auto files = scanner.scan();
    
    bool found_original = false;
    bool found_symlink = false;
    for (const auto& file : files) {
      if (file.path.filename() == "original.txt") found_original = true;
      if (file.path.filename() == "link.txt") found_symlink = true;
    }
    
    EXPECT_TRUE(found_original);
    EXPECT_TRUE(found_symlink);
  }
}

TEST_F(FileScannerTest, HandlesUnicodeFilenames) {
  auto unicode_path = test_dir_ / "测试文件.txt";
  std::ofstream file(unicode_path);
  file << "Unicode test";
  file.close();
  
  FileScanner scanner(test_dir_);
  auto files = scanner.scan();
  
  ASSERT_EQ(files.size(), 1);
  EXPECT_EQ(files[0].path.filename(), "测试文件.txt");
  EXPECT_EQ(files[0].category, "documents");
}

TEST_F(FileScannerTest, SkipsDirectories) {
  create_test_file("file.txt");
  fs::create_directory(test_dir_ / "subdir");
  
  FileScanner scanner(test_dir_);
  auto files = scanner.scan();
  
  ASSERT_EQ(files.size(), 1);
  EXPECT_EQ(files[0].path.filename(), "file.txt");
}

TEST_F(FileScannerTest, HandlesLargeFiles) {
  auto large_file = test_dir_ / "large.bin";
  std::ofstream file(large_file, std::ios::binary);
  
  std::vector<char> data(10 * 1024 * 1024, 'A');
  file.write(data.data(), data.size());
  file.close();
  
  FileScanner scanner(test_dir_);
  auto files = scanner.scan();
  
  ASSERT_EQ(files.size(), 1);
  EXPECT_GT(files[0].size, 10 * 1024 * 1024 - 1);
}
