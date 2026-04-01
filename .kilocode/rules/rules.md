# File Organizer - Project Rules & Context

## Project Overview

**Name:** File Organizer - AI-Powered File Management Tool  
**Language:** C++20  
**Status:** Phase 1 Complete (Week 1/8 of MVP Development)  
**Goal:** Market-ready file organizer with AI-powered duplicate detection

## Current State (2026-03-26)

### ✅ Completed (Phase 1, Week 1)
- **Test Infrastructure:** 42 unit tests, 100% passing, ~85% coverage
- **Build System:** Modern CMake with Debug/Release configs, sanitizers
- **CI/CD:** GitHub Actions for macOS + Ubuntu
- **Benchmarks:** Performance baselines established (75k files/s)
- **Documentation:** TESTING.md, PHASE1_SUMMARY.md, CHANGELOG.md

### 🎯 Active Development
- **Current Phase:** Phase 1, Week 2 - MIME Detection & Robustness
- **Next Milestone:** libmagic integration for content-based classification

### 📋 8-Week MVP Roadmap

**Phase 1: Foundation & Testing (Week 1-2)** ✅ Week 1 Complete
- Week 1: ✅ CMake, Tests, CI/CD, Benchmarks
- Week 2: ⏳ MIME detection, Robustness, Logging

**Phase 2: ONNX & Duplicate Detection (Week 3-5)**
- Week 3: ONNX Runtime setup, Embedding model
- Week 4: Duplicate detection core, Similarity engine
- Week 5: UI integration for duplicates

**Phase 3: Polish & Performance (Week 6-7)**
- Week 6: Persistent cache, Parallelization
- Week 7: UX improvements, Documentation

**Phase 4: MVP Release (Week 8)**
- Week 8: Packaging, Beta launch, Feedback

## Architecture

### Core Components
```
file-organizer/
├── src/
│   ├── file_scanner.cpp    # File discovery & categorization
│   ├── organizer.cpp       # File organization logic
│   ├── config.cpp          # JSON config management
│   ├── ui.cpp              # FTXUI terminal interface
│   └── main.cpp            # Entry point
├── include/                # Public headers
├── tests/                  # Google Test suite (42 tests)
├── benchmarks/             # Google Benchmark suite
└── docs/                   # Documentation
```

### Tech Stack
- **Core:** C++20, CMake 3.20+
- **UI:** FTXUI (terminal GUI)
- **Config:** nlohmann/json
- **Testing:** Google Test, Google Benchmark
- **CI/CD:** GitHub Actions
- **Planned:** ONNX Runtime, SQLite, libmagic, spdlog

## Development Workflow

### Build Commands
```bash
# Debug build with tests
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
cmake --build build --parallel

# Release build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

# With sanitizers
cmake -S . -B build -DENABLE_SANITIZERS=ON -DBUILD_TESTS=ON
cmake --build build --parallel
```

### Testing
```bash
# Run all tests
./build/file_organizer_tests

# Run specific suite
./build/file_organizer_tests --gtest_filter=FileScannerTest.*

# Run benchmarks
./build/file_organizer_bench
```

### Code Style
- Modern C++20: Concepts, Ranges, std::filesystem
- RAII, move semantics, zero-copy where possible
- Compiler flags: `-Wall -Wextra -Wpedantic`
- No warnings tolerated in production code

## Key Design Decisions

### 1. Test-First Approach
- Write tests before/alongside implementation
- Maintain 80%+ coverage
- Use fixtures for setup/teardown

### 2. Performance-Conscious
- Benchmark hot paths
- Target: <5s for 10k files (Release build)
- Use `std::string_view`, `std::span` for zero-copy

### 3. Cross-Platform (macOS + Linux)
- Test on both platforms via CI
- Use `std::filesystem` for portability
- Avoid platform-specific APIs

### 4. AI Integration Strategy
- ONNX Runtime for inference
- Embedding model: all-MiniLM-L6-v2 (384-dim)
- SQLite cache for embeddings
- Duplicate detection via cosine similarity

## Performance Targets

### Current (Debug Build)
- 10k files: 138ms (~75k files/s)
- Extension lookup: 171ns
- FileInfo construction: 6.9μs

### Target (Release Build)
- 10k files: <30ms (>300k files/s)
- Embedding generation: <50ms per file
- Duplicate detection: <5s for 10k files

## Testing Requirements

### Unit Tests
- All public APIs must have tests
- Edge cases: empty dirs, symlinks, permissions
- Error handling: invalid JSON, missing files
- Concepts: verify type constraints work

### Benchmarks
- Scan performance at 10, 100, 1k, 10k files
- Categorization overhead
- Future: ONNX inference latency

## Common Tasks

### Adding a New Feature
1. Write tests first (TDD)
2. Implement in library (`file_organizer_lib`)
3. Update UI if needed
4. Add benchmarks for hot paths
5. Update documentation
6. Verify CI passes

### Debugging
1. Build with Debug + sanitizers
2. Run specific test: `--gtest_filter=TestName`
3. Use `lldb` or `gdb` with debug symbols
4. Check CI logs for platform-specific issues

### Performance Investigation
1. Run benchmarks: `./build/file_organizer_bench`
2. Profile with Instruments (macOS) or perf (Linux)
3. Compare before/after metrics
4. Document findings in commit message

## Dependencies

### Current
- FTXUI v5.0.0 (Terminal UI)
- nlohmann/json v3.11.3 (Config)
- Google Test v1.14.0 (Testing)
- Google Benchmark v1.8.3 (Performance)

### Planned (Phase 2+)
- ONNX Runtime 1.17+ (AI inference)
- Eigen 3.4+ (Linear algebra)
- SQLite 3.40+ (Embedding cache)
- libmagic (MIME detection)
- spdlog 1.12+ (Logging)

## File Organization Categories

### Supported
- Images: jpg, png, gif, svg, webp, heic
- Videos: mp4, avi, mkv, mov
- Audio: mp3, wav, flac, aac
- Documents: pdf, doc, docx, txt, md
- Spreadsheets: xls, xlsx, csv
- Presentations: ppt, pptx
- Code: cpp, py, js, java, rs, go
- Archives: zip, rar, 7z, tar, gz
- Installers: dmg, pkg, exe, msi
- Other: unknown extensions

### Future (MIME-based)
- Content-type detection
- Fallback chain: MIME → Extension → "other"

## Known Issues & Limitations

### Current
- Extension-only categorization (no content analysis)
- No duplicate detection yet
- No conflict resolution for same filenames
- Single-threaded scanning

### Planned Fixes
- Week 2: MIME detection
- Week 4-5: Duplicate detection
- Week 6: Parallelization
- Week 7: Conflict resolution

## Memory & Context

### Important Paths
- **Project Root:** `/Users/saschakohler/Documents/01_Development/Active_Projects/ml-cpp/tasks/file-organizer`
- **Build Dir:** `build/` (gitignored)
- **Tests:** `tests/*.cpp`
- **Benchmarks:** `benchmarks/*.cpp`
- **Docs:** `docs/*.md`

### Key Files
- `CMakeLists.txt` - Build configuration
- `README.md` - User-facing documentation
- `docs/TESTING.md` - Testing guide
- `docs/PHASE1_SUMMARY.md` - Phase 1 completion
- `.windsurf/plans/file-organizer-mvp-bf8546.md` - Full MVP roadmap

### Configuration
- User config: `~/.config/file-organizer/config.json`
- Default watch dir: `~/Downloads`
- Default organize dir: `~/Organized`
- Dry-run enabled by default

## Next Session Checklist

When starting a new conversation:
1. ✅ Read this rules file
2. ✅ Check `docs/PHASE1_SUMMARY.md` for latest status
3. ✅ Review `.windsurf/plans/file-organizer-mvp-bf8546.md` for roadmap
4. ✅ Run tests to verify everything works: `./build/file_organizer_tests`
5. ✅ Check current phase in roadmap (currently: Phase 1, Week 2)
6. ✅ Continue with next task in plan

## Communication Style

- Be concise and technical
- Focus on implementation, not explanations
- Cite files with `@filepath:line` format
- Provide working code, not pseudocode
- Test before claiming completion
- Document decisions in code comments

## Success Metrics

### Phase 1 (Current)
- ✅ 42 tests passing
- ✅ CI green on macOS + Ubuntu
- ✅ Zero compiler warnings
- ✅ No memory leaks (ASan clean)
- ⏳ MIME detection working

### MVP (Week 8)
- 95%+ duplicate detection accuracy
- <5s scan for 10k files
- 50+ beta testers
- 70%+ recommendation rate
- 10+ GitHub stars

## Resources

- **Plan:** `.windsurf/plans/file-organizer-mvp-bf8546.md`
- **Testing:** `docs/TESTING.md`
- **Summary:** `docs/PHASE1_SUMMARY.md`
- **Changelog:** `CHANGELOG.md`
- **Roadmap:** See main roadmap in `.windsurf/workflows/roadmap-rules.md`
