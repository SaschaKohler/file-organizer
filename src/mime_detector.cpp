#include "mime_detector.hpp"
#include <algorithm>
#include <cctype>

MimeDetector::MimeDetector() {
  magic_cookie_ = magic_open(MAGIC_MIME_TYPE);
  if (magic_cookie_ == nullptr) {
    throw std::runtime_error("Failed to initialize libmagic");
  }
  
  if (magic_load(magic_cookie_, nullptr) != 0) {
    magic_close(magic_cookie_);
    throw std::runtime_error("Failed to load magic database");
  }
}

MimeDetector::~MimeDetector() {
  if (magic_cookie_ != nullptr) {
    magic_close(magic_cookie_);
  }
}

MimeDetector::MimeDetector(MimeDetector&& other) noexcept
    : magic_cookie_(other.magic_cookie_) {
  other.magic_cookie_ = nullptr;
}

MimeDetector& MimeDetector::operator=(MimeDetector&& other) noexcept {
  if (this != &other) {
    if (magic_cookie_ != nullptr) {
      magic_close(magic_cookie_);
    }
    magic_cookie_ = other.magic_cookie_;
    other.magic_cookie_ = nullptr;
  }
  return *this;
}

std::optional<std::string> MimeDetector::detect(const fs::path& file_path) const {
  if (!fs::exists(file_path)) {
    return std::nullopt;
  }

  const char* mime = magic_file(magic_cookie_, file_path.c_str());
  if (mime == nullptr) {
    return std::nullopt;
  }

  return std::string(mime);
}

std::string MimeDetector::category_from_mime(const std::string& mime_type) const {
  std::string lower_mime = mime_type;
  std::transform(lower_mime.begin(), lower_mime.end(), lower_mime.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  if (lower_mime.find("image/") == 0) {
    return "images";
  }
  if (lower_mime.find("video/") == 0) {
    return "videos";
  }
  if (lower_mime.find("audio/") == 0) {
    return "audio";
  }
  if (lower_mime.find("text/") == 0) {
    return "documents";
  }
  
  if (lower_mime.find("application/pdf") != std::string::npos ||
      lower_mime.find("application/msword") != std::string::npos ||
      lower_mime.find("application/vnd.openxmlformats-officedocument.wordprocessingml") != std::string::npos ||
      lower_mime.find("application/vnd.oasis.opendocument.text") != std::string::npos ||
      lower_mime.find("application/rtf") != std::string::npos ||
      lower_mime.find("application/x-tex") != std::string::npos ||
      lower_mime.find("application/epub+zip") != std::string::npos ||
      lower_mime.find("application/x-mobipocket-ebook") != std::string::npos) {
    return "documents";
  }
  
  if (lower_mime.find("application/vnd.ms-excel") != std::string::npos ||
      lower_mime.find("application/vnd.openxmlformats-officedocument.spreadsheetml") != std::string::npos ||
      lower_mime.find("application/vnd.oasis.opendocument.spreadsheet") != std::string::npos ||
      lower_mime.find("text/csv") != std::string::npos ||
      lower_mime.find("text/tab-separated-values") != std::string::npos) {
    return "spreadsheets";
  }
  
  if (lower_mime.find("application/vnd.ms-powerpoint") != std::string::npos ||
      lower_mime.find("application/vnd.openxmlformats-officedocument.presentationml") != std::string::npos ||
      lower_mime.find("application/vnd.oasis.opendocument.presentation") != std::string::npos) {
    return "presentations";
  }
  
  if (lower_mime.find("application/zip") != std::string::npos ||
      lower_mime.find("application/x-tar") != std::string::npos ||
      lower_mime.find("application/x-gzip") != std::string::npos ||
      lower_mime.find("application/x-bzip2") != std::string::npos ||
      lower_mime.find("application/x-7z-compressed") != std::string::npos ||
      lower_mime.find("application/x-rar") != std::string::npos ||
      lower_mime.find("application/x-xz") != std::string::npos ||
      lower_mime.find("application/x-lzip") != std::string::npos ||
      lower_mime.find("application/x-lzma") != std::string::npos ||
      lower_mime.find("application/x-compress") != std::string::npos ||
      lower_mime.find("application/x-iso9660-image") != std::string::npos ||
      lower_mime.find("application/x-apple-diskimage") != std::string::npos) {
    return "archives";
  }
  
  if (lower_mime.find("application/x-executable") != std::string::npos ||
      lower_mime.find("application/x-mach-binary") != std::string::npos ||
      lower_mime.find("application/x-deb") != std::string::npos ||
      lower_mime.find("application/x-rpm") != std::string::npos ||
      lower_mime.find("application/vnd.microsoft.portable-executable") != std::string::npos ||
      lower_mime.find("application/x-ms-dos-executable") != std::string::npos ||
      lower_mime.find("application/vnd.android.package-archive") != std::string::npos ||
      lower_mime.find("application/x-debian-package") != std::string::npos ||
      lower_mime.find("application/x-redhat-package-manager") != std::string::npos) {
    return "installers";
  }
  
  if (lower_mime.find("application/javascript") != std::string::npos ||
      lower_mime.find("application/json") != std::string::npos ||
      lower_mime.find("application/xml") != std::string::npos ||
      lower_mime.find("application/x-sh") != std::string::npos ||
      lower_mime.find("application/x-python") != std::string::npos ||
      lower_mime.find("application/x-java") != std::string::npos ||
      lower_mime.find("application/x-ruby") != std::string::npos ||
      lower_mime.find("application/x-php") != std::string::npos ||
      lower_mime.find("application/x-perl") != std::string::npos ||
      lower_mime.find("application/x-httpd-php") != std::string::npos ||
      lower_mime.find("text/x-python") != std::string::npos ||
      lower_mime.find("text/x-java") != std::string::npos ||
      lower_mime.find("text/x-c") != std::string::npos ||
      lower_mime.find("text/x-c++") != std::string::npos ||
      lower_mime.find("text/x-script") != std::string::npos ||
      lower_mime.find("text/html") != std::string::npos) {
    return "code";
  }

  return "other";
}
