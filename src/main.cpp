#include "ui.hpp"
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

int main(int argc, char *argv[]) {
  fs::path config_path = fs::path(std::getenv("HOME")) / ".config" / "file-organizer" / "config.json";
  
  AppConfig config;
  
  auto loaded_config = AppConfig::load_from_file(config_path);
  if (loaded_config) {
    config = *loaded_config;
    std::cout << "Loaded config from: " << config_path << "\n";
  } else {
    config = AppConfig::create_default();
    std::cout << "Using default config. Creating config file...\n";
    
    fs::create_directories(config_path.parent_path());
    config.save_to_file(config_path);
    std::cout << "Config saved to: " << config_path << "\n";
  }
  
  if (argc > 1) {
    config.watch_dir = argv[1];
  }
  
  if (!fs::exists(config.watch_dir)) {
    std::cerr << "Error: Watch directory does not exist: " << config.watch_dir << "\n";
    return 1;
  }
  
  std::cout << "Watch directory: " << config.watch_dir << "\n";
  std::cout << "Organize to: " << config.organize_base_dir << "\n";
  std::cout << "Dry run: " << (config.dry_run ? "YES" : "NO") << "\n";
  std::cout << "\nStarting UI...\n";
  
  FileOrganizerUI ui(config);
  ui.run();
  
  config.save_to_file(config_path);
  
  return 0;
}
