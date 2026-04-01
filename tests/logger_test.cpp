#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include "logger.hpp"

namespace fs = std::filesystem;

class LoggerTest : public ::testing::Test {
protected:
  void SetUp() override {
    test_dir_ = fs::temp_directory_path() / "logger_test";
    fs::create_directories(test_dir_);
    Logger::instance().shutdown();
  }

  void TearDown() override {
    Logger::instance().shutdown();
    if (fs::exists(test_dir_)) {
      fs::remove_all(test_dir_);
    }
  }

  fs::path test_dir_;
};

TEST_F(LoggerTest, InitializeWithDefaults) {
  Logger::instance().init();
  EXPECT_EQ(Logger::instance().get_level(), Logger::Level::Info);
}

TEST_F(LoggerTest, InitializeWithLogFile) {
  auto log_file = test_dir_ / "test.log";
  Logger::instance().init(log_file);
  
  Logger::instance().info("Test message");
  Logger::instance().flush();
  
  EXPECT_TRUE(fs::exists(log_file));
}

TEST_F(LoggerTest, SetAndGetLevel) {
  Logger::instance().init();
  
  Logger::instance().set_level(Logger::Level::Debug);
  EXPECT_EQ(Logger::instance().get_level(), Logger::Level::Debug);
  
  Logger::instance().set_level(Logger::Level::Error);
  EXPECT_EQ(Logger::instance().get_level(), Logger::Level::Error);
}

TEST_F(LoggerTest, LogAtDifferentLevels) {
  auto log_file = test_dir_ / "levels.log";
  Logger::instance().init(log_file, Logger::Level::Trace, false);
  
  Logger::instance().trace("Trace message");
  Logger::instance().debug("Debug message");
  Logger::instance().info("Info message");
  Logger::instance().warn("Warn message");
  Logger::instance().error("Error message");
  Logger::instance().critical("Critical message");
  Logger::instance().flush();
  
  EXPECT_TRUE(fs::exists(log_file));
  EXPECT_GT(fs::file_size(log_file), 0);
}

TEST_F(LoggerTest, LogWithFormatting) {
  auto log_file = test_dir_ / "formatted.log";
  Logger::instance().init(log_file, Logger::Level::Info, false);
  
  Logger::instance().info("Processing {} files in {}", 42, "/path/to/dir");
  Logger::instance().flush();
  
  std::ifstream file(log_file);
  std::string content((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());
  
  EXPECT_TRUE(content.find("Processing 42 files") != std::string::npos);
  EXPECT_TRUE(content.find("/path/to/dir") != std::string::npos);
}

TEST_F(LoggerTest, LevelFilteringWorks) {
  auto log_file = test_dir_ / "filtered.log";
  Logger::instance().init(log_file, Logger::Level::Warn, false);
  
  Logger::instance().debug("Debug message");
  Logger::instance().info("Info message");
  Logger::instance().warn("Warn message");
  Logger::instance().error("Error message");
  Logger::instance().flush();
  
  std::ifstream file(log_file);
  std::string content((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());
  
  EXPECT_TRUE(content.find("Debug message") == std::string::npos);
  EXPECT_TRUE(content.find("Info message") == std::string::npos);
  EXPECT_TRUE(content.find("Warn message") != std::string::npos);
  EXPECT_TRUE(content.find("Error message") != std::string::npos);
}

TEST_F(LoggerTest, MacrosWork) {
  auto log_file = test_dir_ / "macros.log";
  Logger::instance().init(log_file, Logger::Level::Trace, false);
  
  LOG_TRACE("Trace via macro");
  LOG_DEBUG("Debug via macro");
  LOG_INFO("Info via macro");
  LOG_WARN("Warn via macro");
  LOG_ERROR("Error via macro");
  LOG_CRITICAL("Critical via macro");
  Logger::instance().flush();
  
  std::ifstream file(log_file);
  std::string content((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());
  
  EXPECT_TRUE(content.find("Trace via macro") != std::string::npos);
  EXPECT_TRUE(content.find("Critical via macro") != std::string::npos);
}

TEST_F(LoggerTest, ConsoleOutputCanBeDisabled) {
  auto log_file = test_dir_ / "no_console.log";
  Logger::instance().init(log_file, Logger::Level::Info, false);
  
  Logger::instance().info("File only message");
  Logger::instance().flush();
  
  EXPECT_TRUE(fs::exists(log_file));
}

TEST_F(LoggerTest, RotatingFileSink) {
  auto log_file = test_dir_ / "rotating.log";
  Logger::instance().init(log_file, Logger::Level::Info, false);
  
  for (int i = 0; i < 1000; ++i) {
    Logger::instance().info("Message number {} with some padding text to increase size", i);
  }
  Logger::instance().flush();
  
  EXPECT_TRUE(fs::exists(log_file));
}

TEST_F(LoggerTest, ShutdownCleansUp) {
  auto log_file = test_dir_ / "shutdown.log";
  Logger::instance().init(log_file);
  Logger::instance().info("Before shutdown");
  Logger::instance().shutdown();
  
  EXPECT_TRUE(fs::exists(log_file));
}
