#include "config.hpp"
#include <fstream>

void to_json(json& j, const OrganizeRule& rule) {
   j = json{{"category", rule.category},
            {"target_dir", rule.target_dir},
            {"create_subdirs_by_date", rule.create_subdirs_by_date}};
}

void from_json(const json& j, OrganizeRule& rule) {
   j.at("category").get_to(rule.category);
   j.at("target_dir").get_to(rule.target_dir);
   if (j.contains("create_subdirs_by_date")) {
      j.at("create_subdirs_by_date").get_to(rule.create_subdirs_by_date);
   }
}

void to_json(json& j, const AppConfig& config) {
   j = json{{"watch_dir", config.watch_dir.string()},
            {"organize_base_dir", config.organize_base_dir.string()},
            {"quarantine_dir", config.quarantine_dir.string()},
            {"rules", config.rules},
            {"enabled_categories", config.enabled_categories},
            {"dry_run", config.dry_run},
            {"scan_depth", config.scan_depth},
            {"auto_purge_days", config.auto_purge_days}};
}

void from_json(const json& j, AppConfig& config) {
   config.watch_dir = j.at("watch_dir").get<std::string>();
   config.organize_base_dir = j.at("organize_base_dir").get<std::string>();
   if (j.contains("quarantine_dir") &&
       !j.at("quarantine_dir").get<std::string>().empty()) {
      config.quarantine_dir = j.at("quarantine_dir").get<std::string>();
   } else {
      config.quarantine_dir =
          fs::path(std::getenv("HOME")) / ".file-organizer" / "quarantine";
   }
   j.at("rules").get_to(config.rules);
   if (j.contains("enabled_categories")) {
      config.enabled_categories =
          j.at("enabled_categories").get<std::set<std::string>>();
   } else {
      config.enabled_categories = {"images", "videos", "audio", "documents",
                                   "code"};
   }
   config.dry_run = j.value("dry_run", false);
   config.scan_depth = j.value("scan_depth", 0);
   config.auto_purge_days = j.value("auto_purge_days", 0);
}

std::optional<AppConfig>
AppConfig::load_from_file(const fs::path& config_path) {
   if (!fs::exists(config_path)) {
      return std::nullopt;
   }

   std::ifstream file(config_path);
   if (!file.is_open()) {
      return std::nullopt;
   }

   try {
      json j;
      file >> j;
      return j.get<AppConfig>();
   } catch (const json::exception&) {
      return std::nullopt;
   }
}

void AppConfig::save_to_file(const fs::path& config_path) const {
   if (config_path.has_parent_path()) {
      fs::create_directories(config_path.parent_path());
   }

   std::ofstream file(config_path);
   if (!file.is_open()) {
      throw std::runtime_error("Cannot open config file for writing");
   }

   json j = *this;
   file << j.dump(2);
}

AppConfig AppConfig::create_default() {
   AppConfig config;
   config.watch_dir = fs::path(std::getenv("HOME")) / "Downloads";
   config.organize_base_dir = fs::path(std::getenv("HOME")) / "Organized";
   config.quarantine_dir =
       fs::path(std::getenv("HOME")) / ".file-organizer" / "quarantine";
   config.dry_run = true;
   config.scan_depth = 0;
   config.auto_purge_days = 0;
   config.enabled_categories = {"images",    "videos",       "audio",
                                "documents", "spreadsheets", "presentations",
                                "code",      "archives"};

   config.rules = {{.category = "images",
                    .target_dir = "Images",
                    .create_subdirs_by_date = false},
                   {.category = "videos",
                    .target_dir = "Videos",
                    .create_subdirs_by_date = false},
                   {.category = "audio",
                    .target_dir = "Music",
                    .create_subdirs_by_date = false},
                   {.category = "documents",
                    .target_dir = "Documents",
                    .create_subdirs_by_date = true},
                   {.category = "spreadsheets",
                    .target_dir = "Documents/Spreadsheets",
                    .create_subdirs_by_date = false},
                   {.category = "presentations",
                    .target_dir = "Documents/Presentations",
                    .create_subdirs_by_date = false},
                   {.category = "code",
                    .target_dir = "Code",
                    .create_subdirs_by_date = false},
                   {.category = "archives",
                    .target_dir = "Archives",
                    .create_subdirs_by_date = false},
                   {.category = "installers",
                    .target_dir = "Software",
                    .create_subdirs_by_date = false},
                   {.category = "other",
                    .target_dir = "Other",
                    .create_subdirs_by_date = false}};

   return config;
}
