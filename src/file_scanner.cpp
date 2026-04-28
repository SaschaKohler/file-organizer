#include "file_scanner.hpp"
#include <algorithm>
#include <unordered_map>

FileInfo::FileInfo(const fs::path& p)
    : path(p), extension(p.extension().string()), size(fs::file_size(p)),
      modified_time(fs::last_write_time(p)), category("unknown") {}

FileScanner::FileScanner(fs::path root_dir, bool use_mime_detection, int max_depth)
    : root_dir_(std::move(root_dir)), use_mime_detection_(use_mime_detection), max_depth_(max_depth) {
  if (use_mime_detection_) {
    mime_detector_ = std::make_unique<MimeDetector>();
  }
}

void FileScanner::set_use_mime_detection(bool use_mime) {
  use_mime_detection_ = use_mime;
  if (use_mime && !mime_detector_) {
    mime_detector_ = std::make_unique<MimeDetector>();
  }
}

std::string FileScanner::categorize_by_extension(const fs::path& path) const {
  static const std::unordered_map<std::string, std::string> ext_to_category = {
      {".jpg", "images"},
      {".jpeg", "images"},
      {".png", "images"},
      {".gif", "images"},
      {".bmp", "images"},
      {".svg", "images"},
      {".webp", "images"},
      {".heic", "images"},
      {".heif", "images"},
      {".tiff", "images"},
      {".tif", "images"},
      {".ico", "images"},
      {".raw", "images"},
      {".cr2", "images"},
      {".nef", "images"},
      {".arw", "images"},
      {".dng", "images"},
      {".psd", "images"},
      {".ai", "images"},
      {".eps", "images"},
      {".xcf", "images"},

      {".mp4", "videos"},
      {".avi", "videos"},
      {".mkv", "videos"},
      {".mov", "videos"},
      {".wmv", "videos"},
      {".flv", "videos"},
      {".webm", "videos"},
      {".m4v", "videos"},
      {".mpg", "videos"},
      {".mpeg", "videos"},
      {".3gp", "videos"},
      {".ogv", "videos"},
      {".vob", "videos"},
      {".mts", "videos"},
      {".m2ts", "videos"},

      {".mp3", "audio"},
      {".wav", "audio"},
      {".flac", "audio"},
      {".aac", "audio"},
      {".ogg", "audio"},
      {".m4a", "audio"},
      {".wma", "audio"},
      {".opus", "audio"},
      {".ape", "audio"},
      {".alac", "audio"},
      {".aiff", "audio"},
      {".mid", "audio"},
      {".midi", "audio"},

      {".pdf", "documents"},
      {".doc", "documents"},
      {".docx", "documents"},
      {".txt", "documents"},
      {".rtf", "documents"},
      {".odt", "documents"},
      {".md", "documents"},
      {".markdown", "documents"},
      {".tex", "documents"},
      {".epub", "documents"},
      {".mobi", "documents"},
      {".azw", "documents"},
      {".azw3", "documents"},
      {".djvu", "documents"},
      {".pages", "documents"},
      {".wpd", "documents"},
      {".wps", "documents"},
      {".log", "documents"},

      {".xls", "spreadsheets"},
      {".xlsx", "spreadsheets"},
      {".csv", "spreadsheets"},
      {".ods", "spreadsheets"},
      {".numbers", "spreadsheets"},
      {".tsv", "spreadsheets"},
      {".xlsm", "spreadsheets"},
      {".xlsb", "spreadsheets"},

      {".ppt", "presentations"},
      {".pptx", "presentations"},
      {".odp", "presentations"},
      {".key", "presentations"},
      {".pps", "presentations"},
      {".ppsx", "presentations"},
      {".pptm", "presentations"},

      {".zip", "archives"},
      {".rar", "archives"},
      {".7z", "archives"},
      {".tar", "archives"},
      {".gz", "archives"},
      {".bz2", "archives"},
      {".xz", "archives"},
      {".tgz", "archives"},
      {".tbz2", "archives"},
      {".txz", "archives"},
      {".lz", "archives"},
      {".lzma", "archives"},
      {".z", "archives"},
      {".cab", "archives"},
      {".iso", "archives"},
      {".img", "archives"},
      {".dmg", "archives"},

      {".cpp", "code"},
      {".hpp", "code"},
      {".c", "code"},
      {".h", "code"},
      {".cc", "code"},
      {".cxx", "code"},
      {".hxx", "code"},
      {".py", "code"},
      {".pyw", "code"},
      {".pyc", "code"},
      {".js", "code"},
      {".jsx", "code"},
      {".ts", "code"},
      {".tsx", "code"},
      {".java", "code"},
      {".class", "code"},
      {".jar", "code"},
      {".rs", "code"},
      {".go", "code"},
      {".rb", "code"},
      {".php", "code"},
      {".html", "code"},
      {".htm", "code"},
      {".css", "code"},
      {".scss", "code"},
      {".sass", "code"},
      {".less", "code"},
      {".json", "code"},
      {".xml", "code"},
      {".yaml", "code"},
      {".yml", "code"},
      {".toml", "code"},
      {".ini", "code"},
      {".cfg", "code"},
      {".conf", "code"},
      {".sh", "code"},
      {".bash", "code"},
      {".zsh", "code"},
      {".fish", "code"},
      {".ps1", "code"},
      {".bat", "code"},
      {".cmd", "code"},
      {".swift", "code"},
      {".kt", "code"},
      {".kts", "code"},
      {".cs", "code"},
      {".vb", "code"},
      {".sql", "code"},
      {".pl", "code"},
      {".lua", "code"},
      {".r", "code"},
      {".m", "code"},
      {".scala", "code"},
      {".clj", "code"},
      {".dart", "code"},
      {".vue", "code"},
      {".svelte", "code"},

      {".exe", "installers"},
      {".msi", "installers"},
      {".pkg", "installers"},
      {".deb", "installers"},
      {".rpm", "installers"},
      {".apk", "installers"},
      {".ipa", "installers"},
      {".app", "installers"},
      {".appimage", "installers"},
      {".snap", "installers"},
      {".flatpak", "installers"},
  };

  std::string ext = path.extension().string();
  std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

  auto it = ext_to_category.find(ext);
  return it != ext_to_category.end() ? it->second : "other";
}

std::string FileScanner::categorize(const fs::path& path) const {
  if (use_mime_detection_ && mime_detector_) {
    auto mime_type = mime_detector_->detect(path);
    if (mime_type.has_value()) {
      std::string category = mime_detector_->category_from_mime(*mime_type);
      if (category != "other") {
        return category;
      }
    }
  }
  
  return categorize_by_extension(path);
}

void FileScanner::scan_recursive(const fs::path& dir, int current_depth) {
  std::error_code ec;
  
  for (const auto& entry : fs::directory_iterator(dir, fs::directory_options::skip_permission_denied, ec)) {
    if (ec) {
      ec.clear();
      continue;
    }

    std::error_code entry_ec;
    auto status = entry.status(entry_ec);
    if (entry_ec) {
      continue;
    }

    if (status.type() == fs::file_type::directory) {
      if (current_depth < max_depth_) {
        scan_recursive(entry.path(), current_depth + 1);
      }
      continue;
    }

    if (status.type() == fs::file_type::symlink) {
      auto symlink_status = entry.symlink_status(entry_ec);
      if (entry_ec) {
        continue;
      }
      auto target_status = fs::status(entry.path(), entry_ec);
      if (entry_ec || target_status.type() != fs::file_type::regular) {
        continue;
      }
    } else if (status.type() != fs::file_type::regular) {
      continue;
    }

    try {
      FileInfo info(entry.path());
      info.category = categorize(entry.path());
      files_.push_back(std::move(info));
    } catch (const fs::filesystem_error&) {
      continue;
    } catch (const std::exception&) {
      continue;
    }
  }
}

std::vector<FileInfo> FileScanner::scan() {
  files_.clear();

  std::error_code ec;
  if (!fs::exists(root_dir_, ec) || !fs::is_directory(root_dir_, ec)) {
    return files_;
  }

  scan_recursive(root_dir_, 0);

  std::ranges::sort(files_, [](const FileInfo& a, const FileInfo& b) {
    return a.modified_time > b.modified_time;
  });

  return files_;
}
