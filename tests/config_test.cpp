#include "config.hpp"
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

class ConfigTest : public ::testing::Test {
protected:
  fs::path test_config_path_;

  void SetUp() override {
    test_config_path_ = fs::temp_directory_path() / "test_config.json";
  }

  void TearDown() override {
    if (fs::exists(test_config_path_)) {
      fs::remove(test_config_path_);
    }
  }
};

TEST_F(ConfigTest, CreateDefaultConfig) {
  auto config = AppConfig::create_default();
  
  EXPECT_FALSE(config.watch_dir.empty());
  EXPECT_FALSE(config.organize_base_dir.empty());
  EXPECT_TRUE(config.dry_run);
  EXPECT_FALSE(config.rules.empty());
}

TEST_F(ConfigTest, DefaultConfigHasStandardCategories) {
  auto config = AppConfig::create_default();
  
  std::set<std::string> categories;
  for (const auto& rule : config.rules) {
    categories.insert(rule.category);
  }
  
  EXPECT_TRUE(categories.count("images") > 0);
  EXPECT_TRUE(categories.count("videos") > 0);
  EXPECT_TRUE(categories.count("audio") > 0);
  EXPECT_TRUE(categories.count("documents") > 0);
  EXPECT_TRUE(categories.count("code") > 0);
  EXPECT_TRUE(categories.count("archives") > 0);
}

TEST_F(ConfigTest, SaveAndLoadConfig) {
  AppConfig original = AppConfig::create_default();
  original.watch_dir = "/test/watch";
  original.organize_base_dir = "/test/organize";
  original.dry_run = false;
  
  original.save_to_file(test_config_path_);
  
  auto loaded = AppConfig::load_from_file(test_config_path_);
  
  ASSERT_TRUE(loaded.has_value());
  EXPECT_EQ(loaded->watch_dir, original.watch_dir);
  EXPECT_EQ(loaded->organize_base_dir, original.organize_base_dir);
  EXPECT_EQ(loaded->dry_run, original.dry_run);
  EXPECT_EQ(loaded->rules.size(), original.rules.size());
}

TEST_F(ConfigTest, LoadNonExistentConfigReturnsNullopt) {
  fs::path non_existent = test_config_path_.parent_path() / "does_not_exist.json";
  
  auto config = AppConfig::load_from_file(non_existent);
  
  EXPECT_FALSE(config.has_value());
}

TEST_F(ConfigTest, RuleSerialization) {
  OrganizeRule rule;
  rule.category = "images";
  rule.target_dir = "Pictures";
  rule.create_subdirs_by_date = true;
  
  json j = rule;
  
  EXPECT_EQ(j["category"], "images");
  EXPECT_EQ(j["target_dir"], "Pictures");
  EXPECT_EQ(j["create_subdirs_by_date"], true);
}

TEST_F(ConfigTest, RuleDeserialization) {
  json j = {
    {"category", "videos"},
    {"target_dir", "Movies"},
    {"create_subdirs_by_date", false}
  };
  
  OrganizeRule rule = j.get<OrganizeRule>();
  
  EXPECT_EQ(rule.category, "videos");
  EXPECT_EQ(rule.target_dir, "Movies");
  EXPECT_EQ(rule.create_subdirs_by_date, false);
}

TEST_F(ConfigTest, RuleDeserializationWithMissingDateField) {
  json j = {
    {"category", "audio"},
    {"target_dir", "Music"}
  };
  
  OrganizeRule rule = j.get<OrganizeRule>();
  
  EXPECT_EQ(rule.category, "audio");
  EXPECT_EQ(rule.target_dir, "Music");
  EXPECT_FALSE(rule.create_subdirs_by_date);
}

TEST_F(ConfigTest, ConfigSerialization) {
  AppConfig config;
  config.watch_dir = "/home/user/Downloads";
  config.organize_base_dir = "/home/user/Organized";
  config.dry_run = true;
  config.rules = {
    {.category = "images", .target_dir = "Images", .create_subdirs_by_date = false},
    {.category = "documents", .target_dir = "Docs", .create_subdirs_by_date = true}
  };
  
  json j = config;
  
  EXPECT_EQ(j["watch_dir"], "/home/user/Downloads");
  EXPECT_EQ(j["organize_base_dir"], "/home/user/Organized");
  EXPECT_EQ(j["dry_run"], true);
  EXPECT_EQ(j["rules"].size(), 2);
}

TEST_F(ConfigTest, ConfigDeserialization) {
  json j = {
    {"watch_dir", "/test/watch"},
    {"organize_base_dir", "/test/organize"},
    {"dry_run", false},
    {"rules", json::array({
      {{"category", "images"}, {"target_dir", "Pictures"}, {"create_subdirs_by_date", true}},
      {{"category", "videos"}, {"target_dir", "Movies"}, {"create_subdirs_by_date", false}}
    })}
  };
  
  AppConfig config = j.get<AppConfig>();
  
  EXPECT_EQ(config.watch_dir, "/test/watch");
  EXPECT_EQ(config.organize_base_dir, "/test/organize");
  EXPECT_EQ(config.dry_run, false);
  ASSERT_EQ(config.rules.size(), 2);
  EXPECT_EQ(config.rules[0].category, "images");
  EXPECT_EQ(config.rules[1].category, "videos");
}

TEST_F(ConfigTest, SaveCreatesParentDirectories) {
  fs::path nested_path = fs::temp_directory_path() / "nested" / "dir" / "config.json";
  
  AppConfig config = AppConfig::create_default();
  
  EXPECT_NO_THROW(config.save_to_file(nested_path));
  EXPECT_TRUE(fs::exists(nested_path));
  
  fs::remove_all(nested_path.parent_path().parent_path());
}

TEST_F(ConfigTest, LoadInvalidJsonReturnsNullopt) {
  std::ofstream file(test_config_path_);
  file << "{ invalid json }";
  file.close();
  
  auto config = AppConfig::load_from_file(test_config_path_);
  
  EXPECT_FALSE(config.has_value());
}

TEST_F(ConfigTest, RoundTripPreservesData) {
  AppConfig original = AppConfig::create_default();
  original.watch_dir = "/custom/path";
  
  original.save_to_file(test_config_path_);
  auto loaded = AppConfig::load_from_file(test_config_path_);
  
  ASSERT_TRUE(loaded.has_value());
  
  loaded->save_to_file(test_config_path_);
  auto reloaded = AppConfig::load_from_file(test_config_path_);
  
  ASSERT_TRUE(reloaded.has_value());
  EXPECT_EQ(reloaded->watch_dir, original.watch_dir);
  EXPECT_EQ(reloaded->organize_base_dir, original.organize_base_dir);
  EXPECT_EQ(reloaded->dry_run, original.dry_run);
}

TEST_F(ConfigTest, EmptyRulesArray) {
  json j = {
    {"watch_dir", "/test"},
    {"organize_base_dir", "/test2"},
    {"dry_run", true},
    {"rules", json::array()}
  };
  
  AppConfig config = j.get<AppConfig>();
  
  EXPECT_TRUE(config.rules.empty());
}

TEST_F(ConfigTest, ConfigWithMissingDryRunField) {
  json j = {
    {"watch_dir", "/test"},
    {"organize_base_dir", "/test2"},
    {"rules", json::array()}
  };
  
  AppConfig config = j.get<AppConfig>();
  
  EXPECT_FALSE(config.dry_run);
}
