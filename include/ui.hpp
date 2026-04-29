#pragma once

#include "config.hpp"
#include "duplicate_detector.hpp"
#include "file_scanner.hpp"
#include "history_manager.hpp"
#include "mime_detector.hpp"
#include "organizer.hpp"
#include "quarantine.hpp"
#include <ftxui/component/component.hpp>
#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>

class FileOrganizerUI {
 public:
   FileOrganizerUI(AppConfig& config);
   ~FileOrganizerUI();

   FileOrganizerUI(const FileOrganizerUI&) = delete;
   FileOrganizerUI& operator=(const FileOrganizerUI&) = delete;

   void run();

 private:
   AppConfig& config_;
   FileScanner scanner_;
   Organizer organizer_;
   MimeDetector mime_detector_;
   std::unique_ptr<DuplicateDetector> duplicate_detector_;
   Quarantine quarantine_;
   std::unique_ptr<HistoryManager> history_manager_;

   enum class SortMode { Name, Size, Category, Extension };

   enum class UIMode {
      FileList,
      DirectoryBrowser,
      CategorySelector,
      ConfigEditor,
      TextEditor,
      HistoryViewer
   };

   enum class FocusedPanel { FileList, Stats, Preview, CategoryDist };

   enum class ConfigSection { Directories, General, Categories, Rules };

   enum class BrowserSubView { Browse, Favorites, Recent };

   std::vector<FileInfo> files_;
   std::vector<fs::path> directory_entries_;
   std::vector<DuplicateGroup> duplicate_groups_;
   std::vector<std::string> available_categories_;
   int selected_file_ = 0;
   int selected_dir_ = 0;
   int selected_duplicate_group_ = 0;
   int selected_duplicate_file_ = 0;
   int selected_category_ = 0;
   int scroll_offset_ = 0;
   bool show_preview_ = false;
   bool show_duplicates_ = false;
   UIMode ui_mode_ = UIMode::FileList;
   FocusedPanel focused_panel_ = FocusedPanel::FileList;
   fs::path current_browse_path_;
   bool browsing_source_ = true;
   SortMode sort_mode_ = SortMode::Name;
   bool sort_ascending_ = true;
   std::string status_message_;

   // Directory browser state
   BrowserSubView browser_sub_view_ = BrowserSubView::Browse;
   bool creating_folder_ = false;
   std::string new_folder_name_;
   bool browser_search_active_ = false;
   std::string browser_search_query_;
   int selected_quick_access_ = -1;
   int selected_recent_ = 0;
   int selected_favorite_ = 0;

   // Config editor state
   ConfigSection config_section_ = ConfigSection::Directories;
   int selected_config_item_ = 0;
   int selected_rule_ = 0;
   bool editing_rule_ = false;

   // Text editor state
   std::string editing_text_;
   std::string editing_field_name_;
   int text_cursor_pos_ = 0;

   // History viewer state
   std::vector<HistoryEntry> history_entries_;
   int selected_history_ = 0;
   int history_scroll_offset_ = 0;
   std::string history_search_query_;
   bool history_search_active_ = false;
   std::optional<OperationType> history_type_filter_;
   std::optional<OperationStatus> history_status_filter_;

   size_t duplicate_progress_current_ = 0;
   size_t duplicate_progress_total_ = 0;
   std::string duplicate_progress_message_;
   bool duplicate_scan_in_progress_ = false;
   ftxui::ScreenInteractive* screen_ = nullptr;
   std::mutex duplicate_mutex_;
   std::thread duplicate_thread_;
   std::atomic<bool> stop_requested_{false};

   ftxui::Component create_file_list();
   ftxui::Component create_directory_browser();
   ftxui::Component create_duplicate_view();
   ftxui::Component create_category_selector();
   ftxui::Component create_config_editor();
   ftxui::Component create_text_editor();
   ftxui::Component create_controls();
   ftxui::Component create_stats();
   ftxui::Component create_file_details();
   ftxui::Component create_file_preview();
   ftxui::Component create_category_distribution();
   ftxui::Component create_system_status();
   ftxui::Component create_history_view();

   void scan_files();
   void organize_selected();
   void organize_all();
   void toggle_dry_run();
   void toggle_preview();
   void find_duplicates();
   // Quarantine the currently-selected file in the duplicate view.
   void quarantine_selected_duplicate();
   // Quarantine all files in the selected group except the first (oldest).
   void quarantine_group_duplicates();
   // Undo the last quarantine operation.
   void undo_last_quarantine();
   void cycle_sort_mode();
   void toggle_sort_order();
   void sort_files();
   void enter_directory_browser(bool source);
   void exit_directory_browser();
   void browse_directory(const fs::path& path);
   void select_current_directory();
   void navigate_up_directory();
   void browser_jump_to(const fs::path& path);
   void browser_create_folder();
   void browser_toggle_favorite();
   void browser_start_search();
   void browser_cancel_search();
   std::vector<fs::path> get_filtered_entries() const;
   std::vector<std::pair<std::string, fs::path>> get_quick_access_dirs() const;
   void enter_history_view();
   void exit_history_view();
   void history_undo_selected();
   void history_delete_selected();
   void history_export();
   void history_cycle_type_filter();
   void history_cycle_status_filter();
   void history_apply_retention();
   void history_refresh();
   void undo_last_move();
   void update_organizer_base_dir();
   void increase_scan_depth();
   void decrease_scan_depth();
   void enter_category_selector();
   void exit_category_selector();
   void toggle_selected_category();
   bool is_category_enabled(const std::string& category) const;

   void enter_config_editor();
   void exit_config_editor();
   void save_config();
   void toggle_config_category();
   void add_rule();
   void edit_rule();
   void delete_rule();
   void toggle_rule_date_subdirs();

   void enter_text_editor(const std::string& field_name,
                          const std::string& initial_value);
   void exit_text_editor();
   void update_text_editor(char c);
   void delete_char_in_text_editor();
   void apply_text_editor_changes();

   std::string get_file_info_text(const FileInfo& file) const;
   std::string get_mime_type(const FileInfo& file) const;
   std::string get_sort_mode_string() const;
   std::map<std::string, size_t> get_category_stats() const;
   std::vector<fs::path> list_directory(const fs::path& path);
   std::string get_file_preview_content(const FileInfo& file) const;
   ftxui::Element create_progress_bar(float progress, int width) const;
};
