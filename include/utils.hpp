#pragma once

#include <cstdlib>
#include <filesystem>
#include <optional>
#include <pwd.h>
#include <unistd.h>

namespace fs = std::filesystem;

// Returns the user's home directory, checking $HOME first and falling back
// to the passwd database. Returns std::nullopt only if both fail.
inline std::optional<fs::path> get_home_directory() {
  const char* home = std::getenv("HOME");
  if (home && home[0] != '\0') {
    return fs::path(home);
  }

  struct passwd* pw = getpwuid(getuid());
  if (pw && pw->pw_dir) {
    return fs::path(pw->pw_dir);
  }

  return std::nullopt;
}

// Returns the user's home directory or terminates with a clear message.
inline fs::path require_home_directory() {
  auto home = get_home_directory();
  if (!home) {
    std::fprintf(stderr,
                 "Error: Cannot determine home directory. "
                 "Set the HOME environment variable and try again.\n");
    std::exit(1);
  }
  return *home;
}
