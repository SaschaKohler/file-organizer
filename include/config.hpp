#pragma once

#include "organizer.hpp"
#include <nlohmann/json.hpp>
#include <optional>
#include <set>
#include <string>

using json = nlohmann::json;

struct AppConfig {
   fs::path watch_dir;
   fs::path organize_base_dir;
   fs::path quarantine_dir; // Where duplicates are moved when quarantined
   std::vector<OrganizeRule> rules;
   std::set<std::string> enabled_categories;
   bool dry_run = false;
   int scan_depth = 0;
   int auto_purge_days = 0; // 0 = never auto-purge
   int history_retention_days = 30; // 0 = keep forever
   bool history_enabled = true;

   std::vector<std::string> recent_directories;
   std::vector<std::string> favorite_directories;
   static constexpr int MAX_RECENT_DIRS = 5;

   void add_recent_directory(const std::string& dir);

   static std::optional<AppConfig> load_from_file(const fs::path& config_path);
   void save_to_file(const fs::path& config_path) const;

   static AppConfig create_default();
};

void to_json(json& j, const OrganizeRule& rule);
void from_json(const json& j, OrganizeRule& rule);

void to_json(json& j, const AppConfig& config);
void from_json(const json& j, AppConfig& config);
