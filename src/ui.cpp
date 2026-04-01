#include "ui.hpp"
#include "vector_ops.hpp"
#include <algorithm>
#include <fstream>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <iomanip>
#include <sstream>
#include <thread>

using namespace ftxui;

FileOrganizerUI::FileOrganizerUI(AppConfig& config)
    : config_(config), scanner_(config.watch_dir, false, config.scan_depth),
      organizer_(config.organize_base_dir, config.dry_run), embedding_engine_(),
      mime_detector_(), duplicate_detector_(std::make_unique<DuplicateDetector>(
                            embedding_engine_)),
      quarantine_(config.quarantine_dir) {

   for (const auto& rule : config_.rules) {
      organizer_.add_rule(rule.category, rule.target_dir,
                          rule.create_subdirs_by_date);
   }

   available_categories_ = {
       "images",        "videos", "audio",    "documents",  "spreadsheets",
       "presentations", "code",   "archives", "installers", "other"};

   status_message_ = embedding_engine_.is_enabled() ? "ONNX Runtime: Ready"
                                                    : "ONNX Runtime: Disabled";
}

std::string FileOrganizerUI::get_file_info_text(const FileInfo& file) const {
   std::ostringstream oss;
   oss << "Path: " << file.path.string() << "\n"
       << "Size: " << (file.size / 1024.0) << " KB\n"
       << "Category: " << file.category << "\n"
       << "Extension: " << file.extension << "\n"
       << "MIME: " << get_mime_type(file);
   return oss.str();
}

std::string FileOrganizerUI::get_mime_type(const FileInfo& file) const {
   auto mime = mime_detector_.detect(file.path);
   return mime.value_or("unknown");
}

std::optional<std::vector<float>>
FileOrganizerUI::get_embedding(const FileInfo& file) {
   if (!embedding_engine_.is_enabled()) {
      return std::nullopt;
   }
   return embedding_engine_.embed_file(file.path);
}

std::map<std::string, size_t> FileOrganizerUI::get_category_stats() const {
   std::map<std::string, size_t> stats;
   for (const auto& file : files_) {
      stats[file.category]++;
   }
   return stats;
}

std::string
FileOrganizerUI::get_file_preview_content(const FileInfo& file) const {
   std::ifstream ifs(file.path);
   if (!ifs.is_open()) {
      return "[Cannot read file]";
   }

   std::string content;
   std::string line;
   int line_count = 0;
   const int max_lines = 10;
   const int max_line_length = 80;

   while (line_count < max_lines && std::getline(ifs, line)) {
      if (line.length() > max_line_length) {
         line = line.substr(0, max_line_length) + "...";
      }
      content += line + "\n";
      line_count++;
   }

   if (ifs.peek() != EOF) {
      content += "\n[... more content ...]";
   }

   return content.empty() ? "[Empty file]" : content;
}

Element FileOrganizerUI::create_progress_bar(float progress, int width) const {
   int filled = static_cast<int>(progress * width);
   std::string bar;
   for (int i = 0; i < width; ++i) {
      bar += (i < filled) ? "█" : "░";
   }
   return text(bar);
}

void FileOrganizerUI::scan_files() {
   files_ = scanner_.scan();
   sort_files();
   selected_file_ = 0;
   scroll_offset_ = 0;
}

void FileOrganizerUI::organize_selected() {
   if (files_.empty() || selected_file_ >= static_cast<int>(files_.size())) {
      return;
   }

   const auto& file = files_[selected_file_];

   if (!is_category_enabled(file.category)) {
      status_message_ = "Category '" + file.category + "' is disabled";
      return;
   }

   auto result = organizer_.organize_file(file);

   if (result) {
      status_message_ = "Organized: " + file.path.filename().string();
      scan_files();
   } else {
      status_message_ = "No rule for category: " + file.category;
   }
}

void FileOrganizerUI::organize_all() {
   std::vector<FileInfo> files_to_organize;
   for (const auto& file : files_) {
      if (is_category_enabled(file.category)) {
         files_to_organize.push_back(file);
      }
   }
   organizer_.organize_all(files_to_organize);
   scan_files();
}

void FileOrganizerUI::toggle_dry_run() {
   config_.dry_run = !config_.dry_run;
   organizer_.set_dry_run(config_.dry_run);
   status_message_ = config_.dry_run ? "Dry-run enabled" : "Live mode enabled";
}

void FileOrganizerUI::toggle_preview() { show_preview_ = !show_preview_; }

void FileOrganizerUI::find_duplicates() {
   {
      std::lock_guard<std::mutex> lock(duplicate_mutex_);
      show_duplicates_ = true;
      duplicate_scan_in_progress_ = true;
      duplicate_progress_current_ = 0;
      duplicate_progress_total_ = files_.size();
      duplicate_progress_message_ = "Initializing...";
      status_message_ = "Analyzing files for duplicates...";
   }

   auto files_copy = files_;

   std::thread([this, files_copy]() {
      auto progress_callback = [this](size_t current, size_t total,
                                      const std::string& message) {
         {
            std::lock_guard<std::mutex> lock(duplicate_mutex_);
            duplicate_progress_current_ = current;
            duplicate_progress_total_ = total;
            duplicate_progress_message_ = message;
         }
         if (screen_) {
            screen_->PostEvent(Event::Custom);
         }
      };

      auto result =
          duplicate_detector_->find_duplicates(files_copy, progress_callback);

      {
         std::lock_guard<std::mutex> lock(duplicate_mutex_);
         duplicate_groups_ = result;
         duplicate_scan_in_progress_ = false;

         if (duplicate_groups_.empty()) {
            status_message_ = "No duplicates found";
         } else {
            status_message_ = "Found " +
                              std::to_string(duplicate_groups_.size()) +
                              " duplicate groups";
            selected_duplicate_group_ = 0;
            selected_duplicate_file_ = 0;
         }
      }

      if (screen_) {
         screen_->PostEvent(Event::Custom);
      }
   }).detach();
}

void FileOrganizerUI::quarantine_selected_duplicate() {
   if (duplicate_groups_.empty())
      return;
   if (selected_duplicate_group_ >= static_cast<int>(duplicate_groups_.size()))
      return;

   auto& group = duplicate_groups_[selected_duplicate_group_];
   if (group.files.size() < 2)
      return;

   // The "kept" file is the first one in the group; quarantine the selected
   // one.
   const int file_idx = selected_duplicate_file_;
   if (file_idx < 0 || file_idx >= static_cast<int>(group.files.size()))
      return;

   const fs::path& duplicate = group.files[file_idx];
   // Choose the kept file: if the selected IS index 0, keep index 1.
   const fs::path& kept = (file_idx == 0) ? group.files[1] : group.files[0];

   const std::string method =
       (group.similarity_score >= 1.0f) ? "hash" : "simhash";
   auto result = quarantine_.quarantine_file(duplicate, kept,
                                             group.similarity_score, method);

   if (result) {
      status_message_ =
          "Quarantined: " + duplicate.filename().string() + "  (undo: u)";
      // Remove the quarantined file from the group.
      group.files.erase(group.files.begin() + file_idx);
      // If group now has only one file it's no longer a duplicate group.
      if (group.files.size() < 2) {
         duplicate_groups_.erase(duplicate_groups_.begin() +
                                 selected_duplicate_group_);
         selected_duplicate_group_ = std::max(
             0, std::min(selected_duplicate_group_,
                         static_cast<int>(duplicate_groups_.size()) - 1));
      }
      selected_duplicate_file_ = 0;
   } else {
      status_message_ = "Quarantine failed: " + duplicate.filename().string();
   }
}

void FileOrganizerUI::quarantine_group_duplicates() {
   if (duplicate_groups_.empty())
      return;
   if (selected_duplicate_group_ >= static_cast<int>(duplicate_groups_.size()))
      return;

   auto& group = duplicate_groups_[selected_duplicate_group_];
   if (group.files.size() < 2)
      return;

   // Keep the first file, quarantine all others.
   const fs::path& kept = group.files[0];
   const std::string method =
       (group.similarity_score >= 1.0f) ? "hash" : "simhash";
   size_t quarantined = 0;

   // Iterate from the back to preserve indices while erasing.
   for (int i = static_cast<int>(group.files.size()) - 1; i >= 1; --i) {
      auto result = quarantine_.quarantine_file(group.files[i], kept,
                                                group.similarity_score, method);
      if (result) {
         group.files.erase(group.files.begin() + i);
         ++quarantined;
      }
   }

   if (quarantined > 0) {
      status_message_ =
          "Quarantined " + std::to_string(quarantined) + " file(s)  (undo: u)";
      // Group now has only 1 file → remove it.
      duplicate_groups_.erase(duplicate_groups_.begin() +
                              selected_duplicate_group_);
      selected_duplicate_group_ =
          std::max(0, std::min(selected_duplicate_group_,
                               static_cast<int>(duplicate_groups_.size()) - 1));
      selected_duplicate_file_ = 0;
   } else {
      status_message_ = "Quarantine failed for group";
   }
}

void FileOrganizerUI::undo_last_quarantine() {
   if (quarantine_.undo_stack_size() == 0) {
      status_message_ = "Nothing to undo";
      return;
   }
   if (quarantine_.undo_last()) {
      status_message_ = "Restored quarantined file  (" +
                        std::to_string(quarantine_.undo_stack_size()) +
                        " remaining)";
      // Rescan so the restored file appears in the list.
      scan_files();
   } else {
      status_message_ = "Undo failed: quarantined file may be missing";
   }
}

void FileOrganizerUI::undo_last_move() {
   if (organizer_.undo_last_operation()) {
      status_message_ = "Undo successful (" +
                        std::to_string(organizer_.history_size()) +
                        " remaining)";
      scan_files();
   } else {
      status_message_ = "Nothing to undo";
   }
}

void FileOrganizerUI::update_organizer_base_dir() {
   organizer_.set_base_dir(config_.organize_base_dir);
}

void FileOrganizerUI::increase_scan_depth() {
   if (config_.scan_depth < 5) {
      config_.scan_depth++;
      scanner_.set_max_depth(config_.scan_depth);
      status_message_ = "Scan depth: " + std::to_string(config_.scan_depth) +
                        " (" +
                        (config_.scan_depth == 0
                             ? "current dir only"
                             : std::to_string(config_.scan_depth) + " levels") +
                        ")";
      scan_files();
   } else {
      status_message_ = "Maximum scan depth reached (5 levels)";
   }
}

void FileOrganizerUI::decrease_scan_depth() {
   if (config_.scan_depth > 0) {
      config_.scan_depth--;
      scanner_.set_max_depth(config_.scan_depth);
      status_message_ = "Scan depth: " + std::to_string(config_.scan_depth) +
                        " (" +
                        (config_.scan_depth == 0
                             ? "current dir only"
                             : std::to_string(config_.scan_depth) + " levels") +
                        ")";
      scan_files();
   }
}

void FileOrganizerUI::enter_category_selector() {
   ui_mode_ = UIMode::CategorySelector;
   selected_category_ = 0;
   status_message_ = "Select categories to organize";
}

void FileOrganizerUI::exit_category_selector() {
   ui_mode_ = UIMode::FileList;
   status_message_ = std::to_string(config_.enabled_categories.size()) +
                     " categories enabled";
}

void FileOrganizerUI::toggle_selected_category() {
   if (selected_category_ >= 0 &&
       selected_category_ < static_cast<int>(available_categories_.size())) {
      const auto& category = available_categories_[selected_category_];
      if (config_.enabled_categories.count(category)) {
         config_.enabled_categories.erase(category);
         status_message_ = "Disabled: " + category;
      } else {
         config_.enabled_categories.insert(category);
         status_message_ = "Enabled: " + category;
      }
   }
}

void FileOrganizerUI::enter_config_editor() {
   ui_mode_ = UIMode::ConfigEditor;
   config_section_ = ConfigSection::Directories;
   selected_config_item_ = 0;
   selected_rule_ = 0;
   editing_rule_ = false;
   status_message_ = "Config Editor - Use Tab to switch sections";
}

void FileOrganizerUI::exit_config_editor() {
   ui_mode_ = UIMode::FileList;
   status_message_ = "Exited config editor";
}

void FileOrganizerUI::save_config() {
   try {
      auto config_path = fs::path(std::getenv("HOME")) / ".config" /
                         "file-organizer" / "config.json";
      config_.save_to_file(config_path);
      status_message_ = "Configuration saved successfully";
   } catch (const std::exception& e) {
      status_message_ = "Failed to save config: " + std::string(e.what());
   }
}

void FileOrganizerUI::toggle_config_category() {
   if (selected_config_item_ >= 0 &&
       selected_config_item_ < static_cast<int>(available_categories_.size())) {
      const auto& category = available_categories_[selected_config_item_];
      if (config_.enabled_categories.count(category)) {
         config_.enabled_categories.erase(category);
         status_message_ = "Disabled category: " + category;
      } else {
         config_.enabled_categories.insert(category);
         status_message_ = "Enabled category: " + category;
      }
   }
}

void FileOrganizerUI::add_rule() {
   OrganizeRule new_rule;
   new_rule.category = "new_category";
   new_rule.target_dir = "NewFolder";
   new_rule.create_subdirs_by_date = false;
   config_.rules.push_back(new_rule);
   selected_rule_ = config_.rules.size() - 1;
   editing_rule_ = true;
   status_message_ = "Added new rule - press Enter to edit";
}

void FileOrganizerUI::edit_rule() {
   if (selected_rule_ >= 0 &&
       selected_rule_ < static_cast<int>(config_.rules.size())) {
      editing_rule_ = !editing_rule_;
      status_message_ = editing_rule_ ? "Editing rule - use arrow keys"
                                      : "Rule editing disabled";
   }
}

void FileOrganizerUI::delete_rule() {
   if (selected_rule_ >= 0 &&
       selected_rule_ < static_cast<int>(config_.rules.size())) {
      config_.rules.erase(config_.rules.begin() + selected_rule_);
      if (selected_rule_ >= static_cast<int>(config_.rules.size())) {
         selected_rule_ =
             std::max(0, static_cast<int>(config_.rules.size()) - 1);
      }
      status_message_ = "Deleted rule";
   }
}

void FileOrganizerUI::toggle_rule_date_subdirs() {
   if (selected_rule_ >= 0 &&
       selected_rule_ < static_cast<int>(config_.rules.size())) {
      config_.rules[selected_rule_].create_subdirs_by_date =
          !config_.rules[selected_rule_].create_subdirs_by_date;
      status_message_ = "Toggled date subdirs for rule";
   }
}

void FileOrganizerUI::enter_text_editor(const std::string& field_name,
                                        const std::string& initial_value) {
   ui_mode_ = UIMode::TextEditor;
   editing_field_name_ = field_name;
   editing_text_ = initial_value;
   text_cursor_pos_ = static_cast<int>(editing_text_.length());
   status_message_ =
       "Editing " + field_name + " - Enter to save, Esc to cancel";
}

void FileOrganizerUI::exit_text_editor() {
   ui_mode_ = UIMode::ConfigEditor;
   status_message_ = "Cancelled editing";
}

void FileOrganizerUI::update_text_editor(char c) {
   if (c == '\b' || c == 127) { // Backspace
      if (text_cursor_pos_ > 0) {
         editing_text_.erase(static_cast<size_t>(text_cursor_pos_) - 1, 1);
         text_cursor_pos_--;
      }
   } else if (c >= 32 && c <= 126) { // Printable characters
      editing_text_.insert(static_cast<size_t>(text_cursor_pos_), 1, c);
      text_cursor_pos_++;
   }
}

void FileOrganizerUI::delete_char_in_text_editor() {
   if (text_cursor_pos_ < static_cast<int>(editing_text_.length())) {
      editing_text_.erase(static_cast<size_t>(text_cursor_pos_), 1);
   }
}

void FileOrganizerUI::apply_text_editor_changes() {
   if (editing_field_name_ == "watch_dir") {
      config_.watch_dir = editing_text_;
      scanner_ = FileScanner(config_.watch_dir, false, config_.scan_depth);
      status_message_ = "Watch directory updated";
   } else if (editing_field_name_ == "organize_base_dir") {
      config_.organize_base_dir = editing_text_;
      organizer_.set_base_dir(config_.organize_base_dir);
      status_message_ = "Organize directory updated";
   } else if (editing_field_name_ == "quarantine_dir") {
      config_.quarantine_dir = editing_text_;
      quarantine_ = Quarantine(config_.quarantine_dir);
      status_message_ = "Quarantine directory updated";
   } else if (editing_field_name_ == "rule_category") {
      if (selected_rule_ >= 0 &&
          selected_rule_ < static_cast<int>(config_.rules.size())) {
         config_.rules[selected_rule_].category = editing_text_;
         status_message_ = "Rule category updated";
      }
   } else if (editing_field_name_ == "rule_target_dir") {
      if (selected_rule_ >= 0 &&
          selected_rule_ < static_cast<int>(config_.rules.size())) {
         config_.rules[selected_rule_].target_dir = editing_text_;
         status_message_ = "Rule target directory updated";
      }
   }

   exit_text_editor();
}

bool FileOrganizerUI::is_category_enabled(const std::string& category) const {
   return config_.enabled_categories.count(category) > 0;
}

void FileOrganizerUI::cycle_sort_mode() {
   switch (sort_mode_) {
   case SortMode::Name:
      sort_mode_ = SortMode::Size;
      break;
   case SortMode::Size:
      sort_mode_ = SortMode::Category;
      break;
   case SortMode::Category:
      sort_mode_ = SortMode::Extension;
      break;
   case SortMode::Extension:
      sort_mode_ = SortMode::Name;
      break;
   }
   sort_files();
   status_message_ = "Sorted by " + get_sort_mode_string();
}

void FileOrganizerUI::toggle_sort_order() {
   sort_ascending_ = !sort_ascending_;
   sort_files();
   status_message_ =
       std::string(sort_ascending_ ? "Ascending" : "Descending") + " order";
}

void FileOrganizerUI::sort_files() {
   auto compare = [this](const FileInfo& a, const FileInfo& b) {
      bool result;
      switch (sort_mode_) {
      case SortMode::Name:
         result = a.path.filename().string() < b.path.filename().string();
         break;
      case SortMode::Size:
         result = a.size < b.size;
         break;
      case SortMode::Category:
         result = a.category < b.category;
         break;
      case SortMode::Extension:
         result = a.extension < b.extension;
         break;
      }
      return sort_ascending_ ? result : !result;
   };

   std::sort(files_.begin(), files_.end(), compare);
}

std::string FileOrganizerUI::get_sort_mode_string() const {
   switch (sort_mode_) {
   case SortMode::Name:
      return "Name";
   case SortMode::Size:
      return "Size";
   case SortMode::Category:
      return "Category";
   case SortMode::Extension:
      return "Extension";
   }
   return "Unknown";
}

std::vector<fs::path> FileOrganizerUI::list_directory(const fs::path& path) {
   std::vector<fs::path> entries;

   if (!fs::exists(path) || !fs::is_directory(path)) {
      return entries;
   }

   try {
      for (const auto& entry : fs::directory_iterator(path)) {
         if (fs::is_directory(entry.path())) {
            entries.push_back(entry.path());
         }
      }

      std::sort(entries.begin(), entries.end(),
                [](const fs::path& a, const fs::path& b) {
                   return a.filename().string() < b.filename().string();
                });
   } catch (const fs::filesystem_error&) {
      // Ignore permission errors
   }

   return entries;
}

void FileOrganizerUI::enter_directory_browser(bool source) {
   browsing_source_ = source;
   current_browse_path_ =
       browsing_source_ ? config_.watch_dir : config_.organize_base_dir;
   directory_entries_ = list_directory(current_browse_path_);
   selected_dir_ = 0;
   ui_mode_ = UIMode::DirectoryBrowser;
   status_message_ = std::string("Browsing ") +
                     (browsing_source_ ? "source" : "target") + " directory";
}

void FileOrganizerUI::exit_directory_browser() {
   ui_mode_ = UIMode::FileList;
   status_message_ = "Returned to file list";
}

void FileOrganizerUI::browse_directory(const fs::path& path) {
   if (!fs::exists(path) || !fs::is_directory(path)) {
      status_message_ = "Invalid directory";
      return;
   }

   current_browse_path_ = path;
   directory_entries_ = list_directory(current_browse_path_);
   selected_dir_ = 0;
}

void FileOrganizerUI::select_current_directory() {
   if (browsing_source_) {
      config_.watch_dir = current_browse_path_;
      scanner_ = FileScanner(config_.watch_dir, false, config_.scan_depth);
      status_message_ =
          "Source directory updated: " + current_browse_path_.string();
   } else {
      config_.organize_base_dir = current_browse_path_;
      organizer_.set_base_dir(config_.organize_base_dir);
      organizer_.clear_history();
      status_message_ =
          "Target directory updated: " + current_browse_path_.string();
   }
   exit_directory_browser();
   scan_files();
}

void FileOrganizerUI::navigate_up_directory() {
   if (current_browse_path_.has_parent_path() &&
       current_browse_path_ != current_browse_path_.root_path()) {
      browse_directory(current_browse_path_.parent_path());
   }
}

Component FileOrganizerUI::create_directory_browser() {
   auto browser = Container::Vertical({});

   return Renderer(browser, [this] {
      Elements items;

      items.push_back(hbox({
          text("📁 ..") | bold,
          text(" (Parent directory)") | dim,
      }));

      const int visible_lines = 15;
      const int total_dirs = static_cast<int>(directory_entries_.size());

      if (total_dirs > 0) {
         int start = std::max(0, selected_dir_ - visible_lines / 2);
         int end = std::min(total_dirs, start + visible_lines);

         if (end - start < visible_lines && start > 0) {
            start = std::max(0, end - visible_lines);
         }

         for (int i = start; i < end; ++i) {
            const auto& dir = directory_entries_[i];
            auto is_selected = i == selected_dir_;

            auto item = hbox({
                text("📁 "),
                text(dir.filename().string()) | flex,
            });

            if (is_selected) {
               item = item | inverted | bold;
            }

            items.push_back(item);
         }
      }

      auto nav_info = text("[" + std::to_string(selected_dir_ + 1) + "/" +
                           std::to_string(total_dirs) + "]") |
                      dim;

      return vbox({
          text("Directory Browser - " +
               std::string(browsing_source_ ? "Source" : "Target")) |
              bold | center | color(Color::Cyan),
          separator(),
          text("Current: " + current_browse_path_.string()) | dim,
          separator(),
          nav_info,
          separator(),
          vbox(items) | frame | flex,
          separator(),
          hbox({
              text("Enter") | bold,
              text(": Select | "),
              text("Backspace") | bold,
              text(": Up | "),
              text("Esc") | bold,
              text(": Cancel"),
          }) | dim |
              center,
      });
   });
}

Component FileOrganizerUI::create_file_list() {
   auto file_list = Container::Vertical({});

   return Renderer(file_list, [this] {
      Elements items;

      const int total_files = static_cast<int>(files_.size());

      for (int i = 0; i < total_files; ++i) {
         const auto& file = files_[i];
         auto is_selected = i == selected_file_;

         auto item = hbox({
             text(file.path.filename().string()) | flex,
             separator(),
             text(file.category) | size(WIDTH, EQUAL, 15),
             separator(),
             text(std::to_string(file.size / 1024) + " KB") |
                 size(WIDTH, EQUAL, 10),
         });

         if (is_selected) {
            item = item | inverted | bold;
         }

         items.push_back(item);
      }

      auto scroll_info = text("[" + std::to_string(selected_file_ + 1) + "/" +
                              std::to_string(total_files) +
                              "] Sort: " + get_sort_mode_string() + " " +
                              (sort_ascending_ ? "↑" : "↓")) |
                         dim;

      auto title =
          text("Files in " + config_.watch_dir.string()) | bold | center;
      if (focused_panel_ == FocusedPanel::FileList) {
         title = title | color(Color::Yellow);
      }

      auto content = vbox(items);
      if (total_files > 0 && selected_file_ >= 0 &&
          selected_file_ < total_files) {
         content = content | focusPositionRelative(0, selected_file_);
      }

      return vbox({
          title,
          separator(),
          scroll_info,
          separator(),
          content | vscroll_indicator | frame | flex,
      });
   });
}

Component FileOrganizerUI::create_stats() {
   return Renderer([this] {
      auto stats = get_category_stats();
      Elements stat_items;

      for (const auto& [category, count] : stats) {
         stat_items.push_back(hbox({
             text(category) | size(WIDTH, EQUAL, 15),
             separator(),
             text(std::to_string(count)) | align_right,
         }));
      }

      auto title = text("Statistics") | bold | center;
      if (focused_panel_ == FocusedPanel::Stats) {
         title = title | color(Color::Yellow);
      }

      return vbox({
                 title,
                 separator(),
                 vbox(stat_items) | vscroll_indicator,
                 separator(),
                 text("Total: " + std::to_string(files_.size()) + " files"),
             }) |
             border;
   });
}

Component FileOrganizerUI::create_controls() {
   return Renderer([this] {
      return vbox({
                 text("Controls") | bold | center,
                 separator(),
                 text("w/Tab  : Switch panel"),
                 text("↑/↓    : Navigate"),
                 text("Enter  : Organize selected"),
                 text("a      : Organize all"),
                 text("r      : Refresh scan"),
                 text("d      : Toggle dry-run"),
                 text("p      : Toggle preview"),
                 text("f      : Find duplicates"),
                 text("s      : Cycle sort mode"),
                 text("o      : Toggle sort order"),
                 text("b      : Browse source dir"),
                 text("t      : Browse target dir"),
                 text("c      : Select categories"),
                 text("C      : Config editor"),
                 text("u      : Undo last move"),
                 text("+/-    : Scan depth"),
                 text("q      : Quit"),
                 separator(),
                 text(config_.dry_run ? "DRY RUN MODE" : "LIVE MODE") |
                     (config_.dry_run ? color(Color::Yellow)
                                      : color(Color::Green)) |
                     bold,
             }) |
             border;
   });
}

Component FileOrganizerUI::create_file_details() {
   return Renderer([this] {
      if (!show_preview_ || files_.empty() || selected_file_ < 0 ||
          selected_file_ >= static_cast<int>(files_.size())) {
         return vbox({
                    text("File Details") | bold | center,
                    separator(),
                    text("Select a file to view details") | dim | center,
                }) |
                border;
      }

      const auto& file = files_[selected_file_];
      Elements details;

      details.push_back(text("File Details") | bold | center);
      details.push_back(separator());
      details.push_back(text("Name: " + file.path.filename().string()));
      details.push_back(
          text("Size: " + std::to_string(file.size / 1024) + " KB"));
      details.push_back(text("Category: " + file.category));
      details.push_back(text("Extension: " + file.extension));
      details.push_back(text("MIME: " + get_mime_type(file)));

      if (embedding_engine_.is_enabled()) {
         details.push_back(separator());
         details.push_back(text("Embedding: Available") | color(Color::Green));
      }

      return vbox(details) | border;
   });
}

Component FileOrganizerUI::create_file_preview() {
   return Renderer([this] {
      if (files_.empty() || selected_file_ >= static_cast<int>(files_.size())) {
         return vbox({
                    text("File Preview") | bold | color(Color::Cyan),
                    separator(),
                    text("No file selected") | dim | center,
                }) |
                border;
      }

      const auto& file = files_[selected_file_];
      Elements items;

      items.push_back(text("File Preview") | bold | color(Color::Cyan));
      items.push_back(separator());
      items.push_back(text(file.path.filename().string()) | bold);
      items.push_back(
          text("Size: " + std::to_string(file.size / 1024) + " KB") | dim);
      items.push_back(text("Category: " + file.category) | dim);

      auto embedding = get_embedding(file);
      if (embedding.has_value()) {
         items.push_back(text("Embedding: ✓ Available") | color(Color::Green) |
                         dim);
      }

      items.push_back(separator());

      if (file.category == "code" || file.category == "documents" ||
          file.extension == ".txt" || file.extension == ".md") {
         auto preview = get_file_preview_content(file);
         auto lines = std::vector<std::string>();
         std::istringstream stream(preview);
         std::string line;
         while (std::getline(stream, line)) {
            lines.push_back(line);
         }
         for (const auto& l : lines) {
            items.push_back(text(l) | dim);
         }
      } else {
         items.push_back(text("[Binary file - no preview]") | dim | center);
      }

      return vbox(items) | vscroll_indicator | border | flex;
   });
}

Component FileOrganizerUI::create_category_distribution() {
   return Renderer([this] {
      auto stats = get_category_stats();
      size_t total = files_.size();

      Elements items;

      auto title = text("Category Distribution") | bold;
      if (focused_panel_ == FocusedPanel::CategoryDist) {
         title = title | color(Color::Yellow);
      } else {
         title = title | color(Color::Cyan);
      }

      items.push_back(title);
      items.push_back(separator());

      if (total == 0) {
         items.push_back(text("No files scanned") | dim | center);
         return vbox(items) | border;
      }

      std::vector<std::pair<std::string, size_t>> sorted_stats(stats.begin(),
                                                               stats.end());
      std::sort(
          sorted_stats.begin(), sorted_stats.end(),
          [](const auto& a, const auto& b) { return a.second > b.second; });

      for (const auto& [category, count] : sorted_stats) {
         float percentage = (static_cast<float>(count) / total) * 100.0f;
         float progress = static_cast<float>(count) / total;

         bool is_enabled = is_category_enabled(category);

         auto bar = create_progress_bar(progress, 15);
         auto label = text(category);
         auto pct = text(std::to_string(static_cast<int>(percentage)) + "%");
         auto cnt = text(std::to_string(count));

         auto item = hbox({
             label | size(WIDTH, EQUAL, 12),
             text(" "),
             bar,
             text(" "),
             pct | size(WIDTH, EQUAL, 4),
             text(" ("),
             cnt,
             text(")"),
         });

         if (is_enabled) {
            item = item | color(Color::Green);
         } else {
            item = item | dim;
         }

         items.push_back(item);
      }

      items.push_back(separator());
      items.push_back(text("Total: " + std::to_string(total) + " files") | dim);

      return vbox(items) | vscroll_indicator | border;
   });
}

Component FileOrganizerUI::create_system_status() {
   return Renderer([this] {
      Elements status_items;

      auto onnx_status = embedding_engine_.is_enabled()
                             ? text("✓ ONNX") | color(Color::Green)
                             : text("✗ ONNX") | color(Color::Red);

      auto mode_status = config_.dry_run
                             ? text("DRY RUN") | color(Color::Yellow) | bold
                             : text("LIVE") | color(Color::Green) | bold;

      status_items.push_back(hbox({
          text("Status: ") | dim,
          onnx_status,
          text(" | ") | dim,
          text("Depth: " + std::to_string(config_.scan_depth) + "/5") | dim,
          text(" | ") | dim,
          text("Categories: " +
               std::to_string(config_.enabled_categories.size()) + "/10") |
              dim,
          text(" | ") | dim,
          mode_status,
      }));

      status_items.push_back(separator());
      status_items.push_back(text(status_message_) | dim);

      return vbox(status_items) | border;
   });
}

Component FileOrganizerUI::create_config_editor() {
   return Renderer([this] {
      Elements sections;

      // Section tabs
      std::vector<std::string> section_names = {"Directories", "General",
                                                "Categories", "Rules"};
      Elements tabs;
      for (size_t i = 0; i < section_names.size(); ++i) {
         auto is_selected = static_cast<size_t>(config_section_) == i;
         auto tab = text(section_names[i]);
         if (is_selected) {
            tab = tab | inverted | bold;
         }
         tabs.push_back(tab);
         if (i < section_names.size() - 1) {
            tabs.push_back(text(" | ") | dim);
         }
      }
      sections.push_back(hbox(tabs) | center);

      sections.push_back(separator());

      // Content based on selected section
      Elements content;

      if (config_section_ == ConfigSection::Directories) {
         content.push_back(text("Directory Settings") | bold | center);
         content.push_back(separator());

         // Watch directory
         auto watch_selected = selected_config_item_ == 0;
         auto watch_item =
             hbox({text("Watch Dir: ") | (watch_selected ? bold : nothing),
                   text(config_.watch_dir.string()) | flex |
                       (watch_selected ? inverted : nothing)});
         content.push_back(watch_item);

         // Organize base directory
         auto organize_selected = selected_config_item_ == 1;
         auto organize_item = hbox(
             {text("Organize Dir: ") | (organize_selected ? bold : nothing),
              text(config_.organize_base_dir.string()) | flex |
                  (organize_selected ? inverted : nothing)});
         content.push_back(organize_item);

         // Quarantine directory
         auto quarantine_selected = selected_config_item_ == 2;
         auto quarantine_item = hbox(
             {text("Quarantine Dir: ") | (quarantine_selected ? bold : nothing),
              text(config_.quarantine_dir.string()) | flex |
                  (quarantine_selected ? inverted : nothing)});
         content.push_back(quarantine_item);

      } else if (config_section_ == ConfigSection::General) {
         content.push_back(text("General Settings") | bold | center);
         content.push_back(separator());

         // Dry run
         auto dry_run_selected = selected_config_item_ == 0;
         auto dry_run_item =
             hbox({text("Dry Run: ") | (dry_run_selected ? bold : nothing),
                   text(config_.dry_run ? "Enabled" : "Disabled") |
                       (dry_run_selected ? inverted : nothing)});
         content.push_back(dry_run_item);

         // Scan depth
         auto scan_depth_selected = selected_config_item_ == 1;
         auto scan_depth_item = hbox(
             {text("Scan Depth: ") | (scan_depth_selected ? bold : nothing),
              text(std::to_string(config_.scan_depth) + " levels") |
                  (scan_depth_selected ? inverted : nothing)});
         content.push_back(scan_depth_item);

         // Auto purge days
         auto purge_selected = selected_config_item_ == 2;
         auto purge_item = hbox(
             {text("Auto Purge: ") | (purge_selected ? bold : nothing),
              text(config_.auto_purge_days == 0
                       ? "Never"
                       : std::to_string(config_.auto_purge_days) + " days") |
                  (purge_selected ? inverted : nothing)});
         content.push_back(purge_item);

      } else if (config_section_ == ConfigSection::Categories) {
         content.push_back(text("Enabled Categories") | bold | center);
         content.push_back(separator());

         for (size_t i = 0; i < available_categories_.size(); ++i) {
            const auto& category = available_categories_[i];
            bool is_enabled = config_.enabled_categories.count(category) > 0;
            bool is_selected = selected_config_item_ == static_cast<int>(i);

            auto checkbox = text(is_enabled ? "[✓] " : "[ ] ");
            auto label = text(category);

            auto item = hbox({
                checkbox,
                label | flex,
            });

            if (is_selected) {
               item = item | inverted | bold;
            }

            if (is_enabled) {
               item = item | color(Color::Green);
            } else {
               item = item | dim;
            }

            content.push_back(item);
         }

      } else if (config_section_ == ConfigSection::Rules) {
         content.push_back(text("Organization Rules") | bold | center);
         content.push_back(separator());

         if (config_.rules.empty()) {
            content.push_back(text("No rules configured") | dim | center);
         } else {
            for (size_t i = 0; i < config_.rules.size(); ++i) {
               const auto& rule = config_.rules[i];
               bool is_selected = selected_rule_ == static_cast<int>(i);

               auto rule_item =
                   hbox({text("• ") | (is_selected ? bold : nothing),
                         text(rule.category) | size(WIDTH, EQUAL, 12) |
                             (is_selected ? inverted : nothing),
                         text(" → ") | dim,
                         text(rule.target_dir) | flex |
                             (is_selected ? inverted : nothing),
                         text(rule.create_subdirs_by_date ? " (by date)" : "") |
                             dim});

               if (is_selected) {
                  rule_item = rule_item | bold;
               }

               content.push_back(rule_item);
            }
         }
      }

      sections.push_back(vbox(content) | flex);

      sections.push_back(separator());

      // Help text
      Elements help;
      help.push_back(text("Navigation: ↑/↓ Select | Tab: Switch sections | "
                          "Enter: Edit | Space: Toggle") |
                     dim);
      help.push_back(text("Rules: a: Add | e: Edit | d: Delete | s: Toggle "
                          "date subdirs | S: Save") |
                     dim);
      help.push_back(text("Esc: Back to main view") | dim);

      sections.push_back(hbox(help) | center);

      return vbox(sections) | border;
   });
}

Component FileOrganizerUI::create_text_editor() {
   return Renderer([this] {
      Elements items;

      items.push_back(text("Text Editor") | bold | center | color(Color::Cyan));
      items.push_back(separator());
      items.push_back(text("Editing: " + editing_field_name_) | dim | center);
      items.push_back(separator());

      // Display the text with cursor
      std::string display_text = editing_text_;
      if (text_cursor_pos_ >= 0 &&
          text_cursor_pos_ <= static_cast<int>(display_text.length())) {
         display_text.insert(static_cast<size_t>(text_cursor_pos_), "█");
      }

      items.push_back(text(display_text) | flex | center);
      items.push_back(separator());

      Elements help;
      help.push_back(
          text("Type to edit | Backspace: Delete | Delete: Forward delete"));
      help.push_back(text("Enter: Save | Esc: Cancel"));
      help.push_back(text("←/→: Move cursor (not implemented)"));

      items.push_back(hbox(help) | center);

      return vbox(items) | border | center;
   });
}

Component FileOrganizerUI::create_category_selector() {
   return Renderer([this] {
      Elements items;

      items.push_back(text("Select Categories to Organize") | bold | center |
                      color(Color::Cyan));
      items.push_back(separator());
      items.push_back(text("Space: Toggle | Esc: Back") | dim | center);
      items.push_back(separator());

      for (size_t i = 0; i < available_categories_.size(); ++i) {
         const auto& category = available_categories_[i];
         bool is_enabled = config_.enabled_categories.count(category) > 0;
         bool is_selected = static_cast<int>(i) == selected_category_;

         auto checkbox = text(is_enabled ? "[✓] " : "[ ] ");
         auto label = text(category);

         auto item = hbox({
             checkbox,
             label | flex,
         });

         if (is_selected) {
            item = item | inverted | bold;
         }

         if (is_enabled) {
            item = item | color(Color::Green);
         } else {
            item = item | dim;
         }

         items.push_back(item);
      }

      items.push_back(separator());
      items.push_back(text(std::to_string(config_.enabled_categories.size()) +
                           "/" + std::to_string(available_categories_.size()) +
                           " categories enabled") |
                      dim | center);

      return vbox(items) | border | center;
   });
}

Component FileOrganizerUI::create_duplicate_view() {
   return Renderer([this] {
      Elements items;

      bool is_scanning;
      size_t current;
      size_t total;
      std::string message;

      {
         std::lock_guard<std::mutex> lock(duplicate_mutex_);
         is_scanning = duplicate_scan_in_progress_;
         current = duplicate_progress_current_;
         total = duplicate_progress_total_;
         message = duplicate_progress_message_;
      }

      if (is_scanning) {
         float progress =
             total > 0 ? static_cast<float>(current) / total : 0.0f;

         return vbox({
                    text("Duplicate Detection") | bold | center |
                        color(Color::Yellow),
                    separator(),
                    text("Scanning for duplicates...") | center,
                    text("") | flex,
                    hbox({
                        text("Progress: "),
                        create_progress_bar(progress, 50),
                        text(" " + std::to_string(current) + "/" +
                             std::to_string(total)),
                    }) | center,
                    text(message) | dim | center,
                    text("") | flex,
                    text("Please wait...") | dim | center,
                }) |
                border;
      }

      if (duplicate_groups_.empty()) {
         return vbox({
                    text("Duplicate Detection") | bold | center |
                        color(Color::Yellow),
                    separator(),
                    text("No duplicates found") | dim | center,
                    text("") | flex,
                    text("Press Esc to return") | dim | center,
                }) |
                border;
      }

      items.push_back(text("Duplicate Groups: " +
                           std::to_string(duplicate_groups_.size())) |
                      bold);
      items.push_back(separator());

      for (size_t i = 0; i < duplicate_groups_.size(); ++i) {
         const auto& group = duplicate_groups_[i];
         auto is_group_selected =
             static_cast<int>(i) == selected_duplicate_group_;

         auto group_header =
             text("Group " + std::to_string(i + 1) + " (" +
                  std::to_string(
                      static_cast<int>(group.similarity_score * 100)) +
                  "% similar, " + std::to_string(group.files.size()) +
                  " files)") |
             bold;

         if (is_group_selected) {
            group_header = group_header | color(Color::Yellow);
         }

         items.push_back(group_header);

         if (is_group_selected) {
            for (size_t j = 0; j < group.files.size(); ++j) {
               const auto& file = group.files[j];
               auto is_file_selected =
                   static_cast<int>(j) == selected_duplicate_file_;

               uintmax_t size = 0;
               try {
                  size = fs::file_size(file);
               } catch (...) {
               }

               std::string size_str =
                   size < 1024 ? std::to_string(size) + " B"
                   : size < 1024 * 1024
                       ? std::to_string(size / 1024) + " KB"
                       : std::to_string(size / (1024 * 1024)) + " MB";

               auto file_elem = text("  " + file.filename().string() + " (" +
                                     size_str + ")");

               if (is_file_selected) {
                  file_elem = file_elem | inverted;
               } else {
                  file_elem = file_elem | dim;
               }

               items.push_back(file_elem);
            }
         } else {
            items.push_back(text("  " + std::to_string(group.files.size()) +
                                 " files (expand with Enter)") |
                            dim);
         }

         items.push_back(separator());
      }

      return vbox({
                 text("Duplicate Detection") | bold | center |
                     color(Color::Yellow),
                 separator(),
                 vbox(items) | vscroll_indicator | frame | flex,
                 separator(),
                 hbox({
                     text("↑/↓: Navigate | Enter: Expand | x: Quarantine file "
                          "| X: Quarantine group | u: Undo | Esc: Close") |
                         dim,
                 }) | center,
             }) |
             border;
   });
}

void FileOrganizerUI::run() {
   scan_files();

   auto screen = ScreenInteractive::Fullscreen();
   screen_ = &screen;

   auto file_list = create_file_list();
   auto dir_browser = create_directory_browser();
   auto duplicate_view = create_duplicate_view();
   auto category_selector = create_category_selector();
   auto config_editor = create_config_editor();
   auto text_editor = create_text_editor();
   auto stats = create_stats();
   auto controls = create_controls();
   auto file_details = create_file_details();
   auto file_preview = create_file_preview();
   auto category_dist = create_category_distribution();
   auto system_status = create_system_status();

   auto layout = Container::Vertical({
       file_list,
       dir_browser,
       duplicate_view,
       category_selector,
       config_editor,
       text_editor,
       stats,
       controls,
       file_details,
       file_preview,
       category_dist,
       system_status,
   });

   auto main_component = Renderer(layout, [&] {
      if (ui_mode_ == UIMode::DirectoryBrowser) {
         return vbox({
             text("File Organizer - AI Edition") | bold | center |
                 color(Color::Cyan),
             separator(),
             dir_browser->Render() | flex,
         });
      }

      if (ui_mode_ == UIMode::CategorySelector) {
         return vbox({
             text("File Organizer - AI Edition") | bold | center |
                 color(Color::Cyan),
             separator(),
             category_selector->Render() | flex,
         });
      }

      if (ui_mode_ == UIMode::ConfigEditor) {
         return vbox({
             text("File Organizer - AI Edition") | bold | center |
                 color(Color::Cyan),
             separator(),
             config_editor->Render() | flex,
         });
      }

      if (ui_mode_ == UIMode::TextEditor) {
         return vbox({
             text("File Organizer - AI Edition") | bold | center |
                 color(Color::Cyan),
             separator(),
             text_editor->Render() | flex,
         });
      }

      if (show_duplicates_) {
         return vbox({
             text("File Organizer - AI Edition") | bold | center |
                 color(Color::Cyan),
             separator(),
             hbox({
                 file_list->Render() | flex,
                 separator(),
                 duplicate_view->Render() | flex,
             }) | flex,
         });
      }

      auto file_list_elem = file_list->Render() | flex;
      auto stats_elem = stats->Render();
      auto preview_elem = file_preview->Render() | flex;
      auto category_elem = category_dist->Render() | flex;

      if (focused_panel_ == FocusedPanel::FileList) {
         file_list_elem = file_list_elem | borderDouble | color(Color::Yellow);
      } else {
         file_list_elem = file_list_elem | border;
      }

      if (focused_panel_ == FocusedPanel::Stats) {
         stats_elem = stats_elem | borderDouble | color(Color::Yellow);
      }

      if (focused_panel_ == FocusedPanel::Preview) {
         preview_elem = preview_elem | borderDouble | color(Color::Yellow);
      }

      if (focused_panel_ == FocusedPanel::CategoryDist) {
         category_elem = category_elem | borderDouble | color(Color::Yellow);
      }

      return vbox({
          text("File Organizer - AI Edition") | bold | center |
              color(Color::Cyan),
          separator(),
          hbox({
              vbox({
                  file_list_elem,
                  separator(),
                  hbox({
                      preview_elem,
                      separator(),
                      category_elem,
                  }) | size(HEIGHT, EQUAL, 15),
              }) | flex,
              separator(),
              vbox({
                  stats_elem,
                  separator(),
                  controls->Render(),
              }) | size(WIDTH, EQUAL, 40),
          }) | flex,
          separator(),
          system_status->Render(),
      });
   });

   auto component_with_keys = CatchEvent(main_component, [&](Event event) {
      if (ui_mode_ == UIMode::CategorySelector) {
         if (event == Event::Escape) {
            exit_category_selector();
            return true;
         }
         if (event == Event::Character(' ')) {
            toggle_selected_category();
            return true;
         }
         if (event == Event::ArrowUp) {
            selected_category_ = std::max(0, selected_category_ - 1);
            return true;
         }
         if (event == Event::ArrowDown) {
            selected_category_ =
                std::min(static_cast<int>(available_categories_.size()) - 1,
                         selected_category_ + 1);
            return true;
         }
         if (event == Event::Character('q')) {
            screen.ExitLoopClosure()();
            return true;
         }
         return false;
      }

      if (ui_mode_ == UIMode::ConfigEditor) {
         if (event == Event::Escape) {
            exit_config_editor();
            return true;
         }
         if (event == Event::Tab) {
            int next_section = static_cast<int>(config_section_) + 1;
            if (next_section > static_cast<int>(ConfigSection::Rules)) {
               next_section = static_cast<int>(ConfigSection::Directories);
            }
            config_section_ = static_cast<ConfigSection>(next_section);
            selected_config_item_ = 0;
            selected_rule_ = 0;
            status_message_ =
                "Switched to " +
                std::string(config_section_ == ConfigSection::Directories
                                ? "Directories"
                            : config_section_ == ConfigSection::General
                                ? "General"
                            : config_section_ == ConfigSection::Categories
                                ? "Categories"
                                : "Rules");
            return true;
         }
         if (event == Event::ArrowUp) {
            if (config_section_ == ConfigSection::Rules) {
               selected_rule_ = std::max(0, selected_rule_ - 1);
            } else {
               selected_config_item_ = std::max(0, selected_config_item_ - 1);
            }
            return true;
         }
         if (event == Event::ArrowDown) {
            int max_items = config_section_ == ConfigSection::Directories ? 3
                            : config_section_ == ConfigSection::General   ? 3
                            : config_section_ == ConfigSection::Categories
                                ? static_cast<int>(available_categories_.size())
                                : static_cast<int>(config_.rules.size());
            if (config_section_ == ConfigSection::Rules) {
               selected_rule_ = std::min(max_items - 1, selected_rule_ + 1);
            } else {
               selected_config_item_ =
                   std::min(max_items - 1, selected_config_item_ + 1);
            }
            return true;
         }
         if (event == Event::Character(' ')) {
            if (config_section_ == ConfigSection::Categories) {
               toggle_config_category();
            } else if (config_section_ == ConfigSection::General &&
                       selected_config_item_ == 0) {
               config_.dry_run = !config_.dry_run;
               organizer_.set_dry_run(config_.dry_run);
               status_message_ =
                   config_.dry_run ? "Dry-run enabled" : "Live mode enabled";
            }
            return true;
         }
         if (event == Event::Character('a') &&
             config_section_ == ConfigSection::Rules) {
            add_rule();
            return true;
         }
         if (event == Event::Character('d') &&
             config_section_ == ConfigSection::Rules) {
            delete_rule();
            return true;
         }
         if (event == Event::Character('e') &&
             config_section_ == ConfigSection::Rules) {
            edit_rule();
            return true;
         }
         if (event == Event::Character('s') &&
             config_section_ == ConfigSection::Rules) {
            toggle_rule_date_subdirs();
            return true;
         }
         if (event == Event::Return) {
            if (config_section_ == ConfigSection::Directories) {
               if (selected_config_item_ == 0) {
                  enter_text_editor("watch_dir", config_.watch_dir.string());
               } else if (selected_config_item_ == 1) {
                  enter_text_editor("organize_base_dir",
                                    config_.organize_base_dir.string());
               } else if (selected_config_item_ == 2) {
                  enter_text_editor("quarantine_dir",
                                    config_.quarantine_dir.string());
               }
            } else if (config_section_ == ConfigSection::Rules) {
               if (selected_rule_ >= 0 &&
                   selected_rule_ < static_cast<int>(config_.rules.size())) {
                  if (editing_rule_) {
                     // Edit category or target dir
                     enter_text_editor("rule_category",
                                       config_.rules[selected_rule_].category);
                  } else {
                     enter_text_editor(
                         "rule_target_dir",
                         config_.rules[selected_rule_].target_dir);
                  }
               }
            }
            return true;
         }
         if (event == Event::Character('S')) {
            save_config();
            return true;
         }
         if (event == Event::Character('+') || event == Event::Character('=')) {
            if (config_section_ == ConfigSection::General &&
                selected_config_item_ == 1) {
               increase_scan_depth();
            }
            return true;
         }
         if (event == Event::Character('-') || event == Event::Character('_')) {
            if (config_section_ == ConfigSection::General &&
                selected_config_item_ == 1) {
               decrease_scan_depth();
            }
            return true;
         }
         if (event == Event::Character('q')) {
            screen.ExitLoopClosure()();
            return true;
         }
         return false;
      }

      if (ui_mode_ == UIMode::TextEditor) {
         if (event == Event::Escape) {
            exit_text_editor();
            return true;
         }
         if (event == Event::Return) {
            apply_text_editor_changes();
            return true;
         }
         if (event == Event::Backspace) {
            update_text_editor('\b');
            return true;
         }
         if (event == Event::Delete) {
            delete_char_in_text_editor();
            return true;
         }
         if (event.is_character()) {
            std::string chars = event.character();
            if (!chars.empty()) {
               update_text_editor(chars[0]);
            }
            return true;
         }
         return false;
      }

      if (ui_mode_ == UIMode::DirectoryBrowser) {
         if (event == Event::Escape) {
            exit_directory_browser();
            return true;
         }
         if (event == Event::Return) {
            if (selected_dir_ >= 0 &&
                selected_dir_ < static_cast<int>(directory_entries_.size())) {
               browse_directory(directory_entries_[selected_dir_]);
            } else {
               select_current_directory();
            }
            return true;
         }
         if (event == Event::Backspace) {
            navigate_up_directory();
            return true;
         }
         if (event == Event::ArrowUp) {
            selected_dir_ = std::max(-1, selected_dir_ - 1);
            return true;
         }
         if (event == Event::ArrowDown) {
            selected_dir_ =
                std::min(static_cast<int>(directory_entries_.size()) - 1,
                         selected_dir_ + 1);
            return true;
         }
         if (event == Event::Character('q')) {
            screen.ExitLoopClosure()();
            return true;
         }
         return false;
      }

      if (event == Event::Character('q')) {
         screen.ExitLoopClosure()();
         return true;
      }
      if (event == Event::Character('r')) {
         scan_files();
         return true;
      }
      if (event == Event::Character('d')) {
         toggle_dry_run();
         return true;
      }
      if (event == Event::Character('a')) {
         organize_all();
         return true;
      }
      if (event == Event::Character('p')) {
         toggle_preview();
         return true;
      }
      if (event == Event::Tab || event == Event::TabReverse ||
          event == Event::Character('w')) {
         if (!show_duplicates_) {
            switch (focused_panel_) {
            case FocusedPanel::FileList:
               focused_panel_ = FocusedPanel::Stats;
               break;
            case FocusedPanel::Stats:
               focused_panel_ = FocusedPanel::Preview;
               break;
            case FocusedPanel::Preview:
               focused_panel_ = FocusedPanel::CategoryDist;
               break;
            case FocusedPanel::CategoryDist:
               focused_panel_ = FocusedPanel::FileList;
               break;
            }
            return true;
         }
      }
      if (event == Event::Character('f')) {
         if (!show_duplicates_) {
            find_duplicates();
         }
         return true;
      }
      if (show_duplicates_) {
         if (event == Event::Escape) {
            show_duplicates_ = false;
            return true;
         }
         if (event == Event::Tab) {
            show_duplicates_ = false;
            return true;
         }
         if (event == Event::ArrowUp) {
            if (!duplicate_groups_.empty()) {
               if (selected_duplicate_group_ > 0) {
                  selected_duplicate_group_--;
                  selected_duplicate_file_ = 0;
               } else if (selected_duplicate_file_ > 0) {
                  selected_duplicate_file_--;
               }
            }
            return true;
         }
         if (event == Event::ArrowDown) {
            if (!duplicate_groups_.empty()) {
               const auto& current_group =
                   duplicate_groups_[selected_duplicate_group_];
               if (selected_duplicate_file_ <
                   static_cast<int>(current_group.files.size()) - 1) {
                  selected_duplicate_file_++;
               } else if (selected_duplicate_group_ <
                          static_cast<int>(duplicate_groups_.size()) - 1) {
                  selected_duplicate_group_++;
                  selected_duplicate_file_ = 0;
               }
            }
            return true;
         }
         if (event == Event::Return) {
            return true;
         }
         // Quarantine selected duplicate (x) or whole group minus first (X).
         if (event == Event::Character('x')) {
            quarantine_selected_duplicate();
            return true;
         }
         if (event == Event::Character('X')) {
            quarantine_group_duplicates();
            return true;
         }
         // Undo last quarantine operation while in duplicate view.
         if (event == Event::Character('u')) {
            undo_last_quarantine();
            return true;
         }
         return false;
      }
      if (event == Event::Character('s')) {
         cycle_sort_mode();
         return true;
      }
      if (event == Event::Character('o')) {
         toggle_sort_order();
         return true;
      }
      if (event == Event::Character('b')) {
         enter_directory_browser(true);
         return true;
      }
      if (event == Event::Character('t')) {
         enter_directory_browser(false);
         return true;
      }
      if (event == Event::Character('u')) {
         undo_last_move();
         return true;
      }
      if (event == Event::Character('c')) {
         enter_category_selector();
         return true;
      }
      if (event == Event::Character('C')) {
         enter_config_editor();
         return true;
      }
      if (event == Event::Character('+') || event == Event::Character('=')) {
         increase_scan_depth();
         return true;
      }
      if (event == Event::Character('-') || event == Event::Character('_')) {
         decrease_scan_depth();
         return true;
      }
      if (event == Event::Return) {
         organize_selected();
         return true;
      }
      if (event == Event::ArrowUp) {
         if (focused_panel_ == FocusedPanel::FileList) {
            selected_file_ = std::max(0, selected_file_ - 1);
         }
         return true;
      }
      if (event == Event::ArrowDown) {
         if (focused_panel_ == FocusedPanel::FileList) {
            selected_file_ = std::min(static_cast<int>(files_.size()) - 1,
                                      selected_file_ + 1);
         }
         return true;
      }
      return false;
   });

   screen.Loop(component_with_keys);
}
