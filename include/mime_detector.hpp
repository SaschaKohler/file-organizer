#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <magic.h>

namespace fs = std::filesystem;

class MimeDetector {
public:
  MimeDetector();
  ~MimeDetector();

  MimeDetector(const MimeDetector&) = delete;
  MimeDetector& operator=(const MimeDetector&) = delete;
  MimeDetector(MimeDetector&&) noexcept;
  MimeDetector& operator=(MimeDetector&&) noexcept;

  std::optional<std::string> detect(const fs::path& file_path) const;
  
  std::string category_from_mime(const std::string& mime_type) const;

private:
  magic_t magic_cookie_;
};
