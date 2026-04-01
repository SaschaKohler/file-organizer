# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased]

### Added - Phase 1 Week 3 (2026-04-01)

#### Advanced UI Navigation & Responsiveness
- **Tab/w-key panel navigation** - Switch between 4 panels (FileList, Stats, Preview, CategoryDist)
- **Visual focus indicators** - Yellow double borders and titles for focused panels
- **Dynamic terminal adaptation** - File list height adapts to any terminal size (20-200+ lines)
- **Scrollable panels** - All panels now scrollable with vertical scroll indicators
- **Auto-scroll to selection** - `focusPositionRelative` keeps selected item visible
- Removed hardcoded 20-line limit for better UX on large terminals

#### Thread-Safe Duplicate Detection
- **Asynchronous duplicate scanning** - Non-blocking UI during long scans
- **Mutex-protected shared state** - Thread-safe access to progress variables
- **Real-time progress bar** - Live updates with current/total/message
- **Background thread execution** - Detached thread for duplicate detection
- **PostEvent UI refresh** - Proper FTXUI event-driven updates

#### Extended File Format Support (170+ formats)
- **RAW camera formats** - cr2, nef, arw, dng
- **Modern image formats** - heic, heif, webp
- **Ebooks** - epub, mobi, azw, azw3
- **Modern web frameworks** - jsx, tsx, vue, svelte, scss, sass
- **Archives** - tgz, xz, iso, dmg
- **Mobile installers** - apk, ipa, appimage
- **Apple formats** - pages, numbers, key

#### Enhanced MIME Detection
- Better coverage for application/* MIME types
- Code file detection (JavaScript, Python, Java, PHP)
- Ebook detection (EPUB, MOBI)
- Archive detection (ISO, DMG, XZ)
- Installer detection (APK, DEB, RPM)

#### Testing
- **39 new tests** added (73 → 112 total, +53% increase)
- DuplicateDetectorTest suite (18 tests)
- Extended format tests across all categories
- Performance test: 100 files in 42ms
- 100% test pass rate maintained

### Added - Phase 1 Week 2 (2026-03-26)

#### MIME Detection
- `MimeDetector` class for content-based file type detection using libmagic v5.47
- Fallback chain: MIME → Extension → "other" categorization
- Optional MIME detection in FileScanner (disabled by default)
- Runtime toggle for MIME detection via `set_use_mime_detection()`
- 12 comprehensive tests for MIME detection

#### Robustness Improvements
- Graceful handling of filesystem errors using `std::error_code`
- Symlink support (follows symlinks to regular files)
- Permission error handling (skips inaccessible files)
- Unicode filename support (tested with Chinese characters)
- Large file support (tested with 10MB+ files)
- 9 new robustness tests

#### Dependencies
- libmagic v5.47 - MIME type detection (system library)
- pkg-config v2.5.1 - Dependency management

#### Logging System
- `Logger` class - Singleton wrapper around spdlog
- Structured logging levels: Trace, Debug, Info, Warn, Error, Critical, Off
- Log rotation support (10MB file size, 3 backups)
- Console and file output (configurable)
- Convenience macros: LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_CRITICAL
- 10 comprehensive tests for logging functionality

#### Testing
- 31 new tests added (42 → 73 total, +74% increase)
- MimeDetectorTest suite (12 tests)
- Enhanced FileScannerTest suite (9 new tests)
- LoggerTest suite (10 tests)
- 100% test pass rate maintained

### Fixed - Phase 1 Week 2
- FileScanner now gracefully handles permission-denied errors
- Symlinks to regular files are properly followed and categorized
- Filesystem errors no longer crash the scanner
- PNG detection test uses complete minimal PNG file

### Modified - Phase 1 Week 2
- FileScanner constructor accepts optional `use_mime_detection` parameter
- FileScanner::scan() enhanced with comprehensive error handling
- CMakeLists.txt updated with libmagic and spdlog dependencies

### Added - Phase 1 Week 1 (2026-03-26)

#### Testing Infrastructure
- Google Test integration with 42 comprehensive unit tests
- Test suites for FileScanner, Organizer, and Config components
- 100% test pass rate with ~85% code coverage
- Google Benchmark integration for performance testing
- Performance baselines established (75k files/s scan throughput)

#### Build System
- Modernized CMakeLists.txt with Debug/Release configurations
- Debug flags: `-g3 -O0 -Wall -Wextra -Wpedantic`
- Release flags: `-O3 -DNDEBUG -Wall -Wextra`
- Sanitizer support (AddressSanitizer + UndefinedBehaviorSanitizer)
- Shared library (`file_organizer_lib`) for code reuse
- Build options: `BUILD_TESTS`, `BUILD_BENCHMARKS`, `ENABLE_SANITIZERS`

#### CI/CD
- GitHub Actions workflow for automated testing
- Multi-platform support: macOS-latest, Ubuntu-latest
- Multi-configuration: Debug and Release builds
- Sanitizer builds on Ubuntu
- Automated benchmark execution on Release builds

#### Documentation
- `docs/TESTING.md` - Comprehensive testing guide
- `docs/PHASE1_SUMMARY.md` - Phase 1 completion summary
- Updated README with build instructions and project status
- CHANGELOG.md (this file)

#### Bug Fixes
- Fixed `AppConfig::save_to_file` to create parent directories automatically
- Added exception handling for invalid JSON in `AppConfig::load_from_file`
- Added missing headers (`<thread>`, `<chrono>`) in test files

### Performance Metrics (Debug Build)
- 10 files: 0.16ms (64.7k files/s)
- 100 files: 1.19ms (86.3k files/s)
- 1,000 files: 12.7ms (80.9k files/s)
- 10,000 files: 138ms (75.0k files/s)

## [0.1.0] - Initial Release

### Features
- Terminal-based UI using FTXUI
- Automatic file categorization by extension
- Dry-run mode for safe testing
- JSON-based configuration
- Date-based subdirectories (optional)
- Multiple file categories support
- Modern C++20 features (Concepts, Ranges, std::filesystem)

### Supported Categories
- Images (jpg, png, gif, svg, webp, heic, etc.)
- Videos (mp4, avi, mkv, mov, etc.)
- Audio (mp3, wav, flac, aac, etc.)
- Documents (pdf, doc, docx, txt, md, etc.)
- Spreadsheets (xls, xlsx, csv, etc.)
- Presentations (ppt, pptx, etc.)
- Code (cpp, py, js, java, etc.)
- Archives (zip, rar, 7z, tar, gz, etc.)
- Installers (dmg, pkg, exe, msi, etc.)
- Other (unknown extensions)
