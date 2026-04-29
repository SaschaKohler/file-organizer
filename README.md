# File Organizer

> **Also known as:** `forg` / `file-forg` / `tidy-cli` — a smart file manager for the terminal

A fast, modern CLI tool for automatically organizing files in directories. Features a beautiful TUI with real-time preview, duplicate detection, and intelligent categorization.

![File Organizer TUI Screenshot](Screenshot%202026-04-29%20at%2011.08.00.png)

*Interactive TUI showing file listing, statistics panel, and keyboard shortcuts*

## Features

- **Automatic categorization** by file type (170+ file formats)
- **Modern terminal GUI** with FTXUI
  - Tab navigation between panels
  - Dynamic terminal adaptation
  - Scrollable panels with visual indicators
  - Focus highlighting (yellow borders & titles)
- **Live preview** of files to be organized
- **Dry-run mode** for safe testing
- **JSON configuration** for flexible rules
- **Statistics** by category
- **Date-based subfolders** (optional)
- **Duplicate detection** with real-time progress
- **Thread-safe** asynchronous processing
- **Structured logging** with spdlog
- **MIME-based detection** with libmagic
- **Batch mode** for scripting and CI/CD pipelines
- **Signal handling** for clean shutdown (Ctrl+C safe)
- **Conflict resolution** — never overwrites existing files

## Quick Start

```bash
# Clone the repository
git clone https://github.com/SaschaKohler/file-organizer.git
cd file-organizer

# Build (Release)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

# Run
./build/file_organizer
```

## Platform Support

| Platform | Status | Notes |
|----------|--------|-------|
| **macOS** | ✅ Supported | 12.0+ (Monterey and later) |
| **Linux** | ✅ Supported | Ubuntu 22.04+, Debian 12+, Fedora 39+ |
| Windows | 🚧 Planned | Not yet available — expected in a future release |

Both macOS and Linux are fully supported with CI/CD testing on GitHub Actions.

## Build

### Development Build (with tests)
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
cmake --build build --parallel
./build/file_organizer_tests
```

### With Sanitizers
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON
cmake --build build --parallel
```

### Install
```bash
cmake --install build --prefix /usr/local
```

### Dependencies

**Required (system):**
- CMake 3.20+
- C++20 compiler (GCC 11+, Clang 14+, AppleClang 15+)
- libmagic (`brew install libmagic` on macOS, `apt install libmagic-dev` on Ubuntu)
- pkg-config

**Fetched automatically via CMake:**
- FTXUI v5.0.0 — Terminal UI
- nlohmann/json v3.11.3 — JSON parsing
- spdlog v1.12.0 — Structured logging
- Eigen 3.4.0 — Linear algebra
- CLI11 v2.4.1 — Command-line parsing
- Google Test v1.14.0 — Unit testing (optional)
- Google Benchmark v1.8.3 — Performance testing (optional)

## Usage

```bash
# Interactive TUI with default directory (~/Downloads)
file-organizer

# Organize a specific directory
file-organizer /path/to/directory

# Batch mode (no TUI — organize and exit)
file-organizer --batch /path/to/directory

# Dry run in batch mode (preview without moving)
file-organizer --batch --dry-run /path/to/directory

# Custom config file
file-organizer --config /path/to/config.json

# Set scan depth
file-organizer --depth 3 /path/to/directory

# Show version
file-organizer --version

# Show help
file-organizer --help
```

### CLI Options

| Flag | Short | Description |
|------|-------|-------------|
| `--help` | `-h` | Show help message |
| `--version` | `-v` | Show version |
| `--dry-run` | `-n` | Preview changes without moving files |
| `--batch` | `-b` | Run in batch mode (no TUI) |
| `--config <path>` | `-c` | Path to config file |
| `--depth <N>` | `-d` | Scan depth (0 = current dir only) |
| `--verbose` | | Enable verbose logging |
| `--quiet` | `-q` | Suppress non-essential output |

## Keyboard Controls (TUI Mode)

### Navigation
- `w` / `Tab` — Switch between panels (FileList, Stats, Preview, CategoryDist)
- `↑/↓` — Navigate through files (in FileList panel)
- `Esc` — Back to main menu (from duplicate/browser view)

### Actions
- `Enter` — Organize selected file
- `a` — Organize all files
- `r` — Refresh / rescan
- `d` — Toggle dry-run mode
- `p` — Toggle preview panel
- `f` — Start duplicate detection

### Advanced
- `s` — Cycle sort mode (Name/Size/Category/Extension)
- `o` — Reverse sort order
- `b` — Browse source directory
- `t` — Browse target directory
- `c` — Select categories
- `u` — Undo last action
- `+/-` — Adjust scan depth
- `q` — Quit

## Configuration

Configuration is stored in `~/.config/file-organizer/config.json`. See [`config.json.example`](config.json.example) for the full format:

```json
{
  "watch_dir": "~/Downloads",
  "organize_base_dir": "~/Organized",
  "dry_run": true,
  "rules": [
    {
      "category": "images",
      "target_dir": "Images",
      "create_subdirs_by_date": false
    },
    {
      "category": "documents",
      "target_dir": "Documents",
      "create_subdirs_by_date": true
    }
  ]
}
```

## Supported Categories

- **Images**: jpg, png, gif, svg, webp, heic, heif, cr2, nef, arw, dng (RAW), psd, etc.
- **Videos**: mp4, avi, mkv, mov, webm, flv, etc.
- **Audio**: mp3, wav, flac, aac, ogg, m4a, etc.
- **Documents**: pdf, doc, docx, txt, md, rtf, odt, epub, mobi, azw, etc.
- **Spreadsheets**: xls, xlsx, csv, ods, numbers, etc.
- **Presentations**: ppt, pptx, odp, key, etc.
- **Code**: cpp, py, js, java, ts, tsx, jsx, vue, svelte, go, rust, etc.
- **Archives**: zip, rar, 7z, tar, gz, tgz, xz, iso, dmg, etc.
- **Installers**: dmg, pkg, exe, msi, deb, rpm, apk, ipa, appimage, etc.
- **Other**: Everything else

**170+ file formats** with MIME-based detection for maximum accuracy.

## Safety

- **Dry-run mode** is enabled by default
- Files are moved, never deleted
- Target directories are created automatically
- **Filename conflicts** are resolved by appending a counter (`file (1).pdf`, `file (2).pdf`)
- **Signal handling** ensures clean shutdown during file operations

## Testing

The project has comprehensive tests:

- **112+ unit tests** (Google Test)
- **Performance benchmarks** (Google Benchmark)
- **CI/CD pipeline** (GitHub Actions for macOS + Linux)

```bash
# Run tests
./build/file_organizer_tests

# Run benchmarks
./build/file_organizer_bench
```

See `docs/TESTING.md` for details.

## C++20 Features

- **Concepts**: `PathLike`, `Organizable` for type constraints
- **Ranges**: `std::views::filter` for functional operations
- **std::filesystem**: Modern filesystem operations
- **std::optional**: Safe return values
- **Template constraints**: Type-safe generic functions
- **Structured bindings**: `auto [category, count]`
- **std::ranges::sort**: Modern sorting

## License

[MIT](LICENSE)
