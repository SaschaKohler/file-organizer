#include "config.hpp"
#include "file_scanner.hpp"
#include "logger.hpp"
#include "organizer.hpp"
#include "ui.hpp"
#include "utils.hpp"
#include <CLI/CLI.hpp>
#include <atomic>
#include <csignal>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

#define FILE_ORGANIZER_VERSION "0.3.0"

static std::atomic<bool> g_interrupted{false};

static void signal_handler(int /*signum*/) { g_interrupted = true; }

static void install_signal_handlers() {
  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);
}

static int run_batch(AppConfig& config) {
  FileScanner scanner(config.watch_dir, false, config.scan_depth);
  Organizer organizer(config.organize_base_dir, config.dry_run);

  for (const auto& rule : config.rules) {
    organizer.add_rule(rule.category, rule.target_dir,
                       rule.create_subdirs_by_date);
  }

  auto files = scanner.scan();
  if (files.empty()) {
    std::cout << "No files found in: " << config.watch_dir << "\n";
    return 0;
  }

  std::cout << "Found " << files.size() << " file(s) in "
            << config.watch_dir << "\n";

  if (config.dry_run) {
    std::cout << "[DRY RUN] The following files would be organized:\n";
  }

  size_t organized = 0;
  for (const auto& file : files) {
    if (g_interrupted) {
      std::cerr << "\nInterrupted. " << organized
                << " file(s) organized before interruption.\n";
      return 130;
    }

    auto result = organizer.organize_file(file);
    if (result) {
      std::cout << file.path.filename().string() << " -> "
                << result->string() << "\n";
      ++organized;
    }
  }

  std::cout << (config.dry_run ? "[DRY RUN] Would organize " : "Organized ")
            << organized << " of " << files.size() << " file(s).\n";
  return 0;
}

int main(int argc, char* argv[]) {
  install_signal_handlers();

  CLI::App app{"File Organizer - Automatically organize files by category",
               "file-organizer"};
  app.set_version_flag("-v,--version", FILE_ORGANIZER_VERSION);

  std::string directory;
  std::string config_file;
  bool dry_run = false;
  bool dry_run_set = false;
  bool batch = false;
  int depth = -1;
  bool verbose = false;
  bool quiet = false;

  app.add_option("directory", directory,
                 "Directory to organize (overrides config)");
  app.add_option("-c,--config", config_file, "Path to config file");
  app.add_flag("-n,--dry-run", dry_run, "Preview changes without moving files")
      ->each([&dry_run_set](const std::string&) { dry_run_set = true; });
  app.add_flag("-b,--batch", batch,
               "Run in batch mode (no TUI, organize and exit)");
  app.add_option("-d,--depth", depth, "Scan depth (0 = current dir only)");
  app.add_flag("--verbose", verbose, "Enable verbose logging");
  app.add_flag("-q,--quiet", quiet, "Suppress non-essential output");

  CLI11_PARSE(app, argc, argv);

  fs::path config_path;
  if (!config_file.empty()) {
    config_path = config_file;
  } else {
    config_path =
        require_home_directory() / ".config" / "file-organizer" / "config.json";
  }

  AppConfig config;
  auto loaded_config = AppConfig::load_from_file(config_path);
  if (loaded_config) {
    config = *loaded_config;
    if (!quiet) {
      std::cout << "Loaded config from: " << config_path << "\n";
    }
  } else {
    config = AppConfig::create_default();
    if (!quiet) {
      std::cout << "Using default config. Creating config file...\n";
    }
    fs::create_directories(config_path.parent_path());
    config.save_to_file(config_path);
    if (!quiet) {
      std::cout << "Config saved to: " << config_path << "\n";
    }
  }

  if (!directory.empty()) {
    config.watch_dir = directory;
  }
  if (dry_run_set) {
    config.dry_run = dry_run;
  }
  if (depth >= 0) {
    config.scan_depth = depth;
  }

  if (verbose) {
    Logger::instance().init("", Logger::Level::Debug);
  }

  if (!fs::exists(config.watch_dir)) {
    std::cerr << "Error: Watch directory does not exist: " << config.watch_dir
              << "\n";
    return 1;
  }

  if (batch) {
    return run_batch(config);
  }

  if (!quiet) {
    std::cout << "Watch directory: " << config.watch_dir << "\n";
    std::cout << "Organize to: " << config.organize_base_dir << "\n";
    std::cout << "Dry run: " << (config.dry_run ? "YES" : "NO") << "\n";
    std::cout << "\nStarting UI...\n";
  }

  FileOrganizerUI ui(config);
  ui.run();

  config.save_to_file(config_path);

  return 0;
}
