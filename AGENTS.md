# AI Coding Agents - File Organizer Project

## Purpose

This file provides context and guidelines for AI coding assistants (like Cascade in Windsurf) working on the File Organizer project. It ensures consistency, quality, and continuity across development sessions.

---

## Project Context

### What is File Organizer?
An AI-powered file management tool written in C++20 that automatically organizes files by category and detects duplicates using machine learning embeddings.

### Current Status
- **Phase:** 1 of 4 (Foundation & Testing)
- **Week:** 1 of 8 complete ✅ → Week 2 starting ⏳
- **Tests:** 42/42 passing
- **CI/CD:** Green on macOS + Ubuntu
- **Next Milestone:** MIME detection integration

### Key Goals
1. **Week 8:** Market-ready MVP with duplicate detection
2. **Performance:** <5s to scan 10k files (Release build)
3. **Quality:** 80%+ test coverage, zero warnings, no memory leaks
4. **Cross-platform:** macOS + Linux support

---

## Essential Files to Read First

When starting a new session, **always read these files in order:**

1. **`.windsurf/rules.md`** - Project rules, architecture, tech stack
2. **`docs/PROJECT_STATE.md`** - Current status, recent changes, next tasks
3. **`.windsurf/plans/file-organizer-mvp-bf8546.md`** - Full 8-week roadmap
4. **`QUICKSTART.md`** - Quick commands and verification steps

---

## Development Principles

### 1. Test-Driven Development (TDD)
- **Always** write tests before or alongside implementation
- Maintain 80%+ code coverage
- Use Google Test framework
- Run tests after every change: `./build/file_organizer_tests`

### 2. Performance-Conscious
- Benchmark hot paths with Google Benchmark
- Target: 75k+ files/s scan throughput
- Use zero-copy patterns (`std::string_view`, `std::span`)
- Profile before optimizing

### 3. Modern C++20
- Use Concepts for type constraints
- Use Ranges for functional operations
- Use `std::filesystem` for portability
- RAII for resource management
- Move semantics where appropriate

### 4. Code Quality
- **Zero compiler warnings** (`-Wall -Wextra -Wpedantic`)
- **Zero memory leaks** (verified with ASan)
- **Zero undefined behavior** (verified with UBSan)
- Clean, readable code over clever code

### 5. Documentation
- Update docs alongside code changes
- Keep `docs/PROJECT_STATE.md` current
- Update `CHANGELOG.md` for significant changes
- Comment complex algorithms, not obvious code

---

## Workflow Guidelines

### Starting a Session

```bash
# 1. Navigate to project
cd /Users/saschakohler/Documents/01_Development/Active_Projects/ml-cpp/tasks/file-organizer

# 2. Read essential files
cat .windsurf/rules.md
cat docs/PROJECT_STATE.md

# 3. Verify build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
cmake --build build --parallel
./build/file_organizer_tests

# 4. Check current phase and continue
```

### Making Changes

1. **Understand the requirement** - Read related code and tests
2. **Write test first** - Create failing test for new functionality
3. **Implement** - Write minimal code to pass the test
4. **Verify** - Run all tests, check with sanitizers
5. **Benchmark** - If performance-critical, measure impact
6. **Document** - Update relevant documentation
7. **Commit** - Descriptive commit message

### Before Committing

- [ ] All tests pass: `./build/file_organizer_tests`
- [ ] No compiler warnings
- [ ] Sanitizers clean (build with `-DENABLE_SANITIZERS=ON`)
- [ ] Benchmarks run (if performance-related)
- [ ] Documentation updated
- [ ] `CHANGELOG.md` updated

---

## Code Style & Conventions

### Naming
- **Classes:** `PascalCase` (e.g., `FileScanner`)
- **Functions/Methods:** `snake_case` (e.g., `scan_files()`)
- **Variables:** `snake_case` (e.g., `file_count`)
- **Constants:** `UPPER_SNAKE_CASE` (e.g., `MAX_FILES`)
- **Private members:** `trailing_underscore_` (e.g., `root_dir_`)

### File Organization
```cpp
// Header (.hpp)
#pragma once
#include <system_headers>
#include "local_headers.hpp"

namespace fs = std::filesystem;

class MyClass {
public:
  // Public interface
private:
  // Private implementation
};
```

### Error Handling
- Use `std::optional` for operations that may fail
- Use exceptions for exceptional conditions
- Validate inputs at API boundaries
- Provide meaningful error messages

### Comments
```cpp
// Good: Explain WHY, not WHAT
// Use cosine similarity because it's scale-invariant
auto similarity = cosine_distance(a, b);

// Bad: Obvious comment
// Calculate cosine distance
auto similarity = cosine_distance(a, b);
```

---

## Testing Strategy

### Unit Tests
- Test each public method
- Test edge cases (empty input, null, max values)
- Test error conditions
- Use fixtures for setup/teardown

### Test Structure
```cpp
TEST_F(ComponentTest, DescriptiveName) {
  // Arrange
  Component comp(params);
  
  // Act
  auto result = comp.do_something();
  
  // Assert
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, expected);
}
```

### Benchmarks
```cpp
static void BM_Operation(benchmark::State& state) {
  Component comp;
  for (auto _ : state) {
    comp.operation();
    benchmark::DoNotOptimize(comp);
  }
}
BENCHMARK(BM_Operation);
```

---

## Common Tasks

### Adding a New Feature

1. Create test file: `tests/feature_test.cpp`
2. Write failing tests
3. Create header: `include/feature.hpp`
4. Create implementation: `src/feature.cpp`
5. Update `CMakeLists.txt` (add to `file_organizer_lib`)
6. Implement until tests pass
7. Add benchmarks if performance-critical
8. Update documentation

### Fixing a Bug

1. Write test that reproduces the bug
2. Verify test fails
3. Fix the bug
4. Verify test passes
5. Run all tests
6. Check with sanitizers
7. Update `CHANGELOG.md`

### Optimizing Performance

1. Run benchmarks to establish baseline
2. Profile to identify bottleneck
3. Optimize the hot path
4. Re-run benchmarks
5. Verify tests still pass
6. Document improvement in commit

---

## Architecture Overview

### Components

```
FileScanner
├─ Scans directories
├─ Categorizes files by extension
└─ Returns FileInfo objects

Organizer
├─ Applies organization rules
├─ Moves files to target directories
└─ Supports dry-run mode

Config
├─ Loads/saves JSON configuration
├─ Manages user preferences
└─ Provides defaults

UI (FTXUI)
├─ Terminal-based interface
├─ Interactive file selection
└─ Real-time statistics
```

### Future Components (Phase 2+)

```
MimeDetector (Week 2)
├─ Content-based file type detection
└─ Fallback to extension

EmbeddingEngine (Week 3-4)
├─ ONNX Runtime integration
├─ Generate file embeddings
└─ Cache results in SQLite

DuplicateDetector (Week 4-5)
├─ Cosine similarity calculation
├─ Clustering similar files
└─ Hash-based exact duplicates

Logger (Week 2)
├─ spdlog integration
├─ Structured logging
└─ Log rotation
```

---

## Dependencies

### Current
- **FTXUI** v5.0.0 - Terminal UI
- **nlohmann/json** v3.11.3 - JSON parsing
- **Google Test** v1.14.0 - Testing
- **Google Benchmark** v1.8.3 - Performance

### Planned
- **ONNX Runtime** 1.17+ (Week 3)
- **libmagic** (Week 2)
- **spdlog** 1.12+ (Week 2)
- **SQLite** 3.40+ (Week 6)
- **Eigen** 3.4+ (Week 4)

---

## Build System

### CMake Options
```cmake
-DCMAKE_BUILD_TYPE=Debug|Release    # Build configuration
-DBUILD_TESTS=ON|OFF                # Enable tests
-DBUILD_BENCHMARKS=ON|OFF           # Enable benchmarks
-DENABLE_SANITIZERS=ON|OFF          # Enable ASan/UBSan
```

### Build Targets
- `file_organizer` - Main executable
- `file_organizer_lib` - Shared library
- `file_organizer_tests` - Test suite
- `file_organizer_bench` - Benchmarks

---

## Debugging Tips

### Memory Issues
```bash
# Build with sanitizers
cmake -S . -B build -DENABLE_SANITIZERS=ON
cmake --build build
./build/file_organizer_tests
```

### Test Failures
```bash
# Run specific test with verbose output
./build/file_organizer_tests --gtest_filter=FailingTest.* --gtest_verbose

# Run with debugger
lldb ./build/file_organizer_tests
(lldb) run --gtest_filter=FailingTest.*
```

### Performance Issues
```bash
# Run benchmarks
./build/file_organizer_bench

# Profile on macOS
instruments -t "Time Profiler" ./build/file_organizer

# Profile on Linux
perf record ./build/file_organizer
perf report
```

---

## CI/CD

### GitHub Actions
- Runs on every push/PR
- Tests on macOS-latest + Ubuntu-latest
- Tests Debug + Release configurations
- Runs sanitizers on Ubuntu
- Runs benchmarks on Release builds

### Local Pre-Push Checks
```bash
# Test both configurations
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
cmake --build build-debug && ./build-debug/file_organizer_tests

cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
cmake --build build-release && ./build-release/file_organizer_tests

# Test with sanitizers
cmake -S . -B build-san -DENABLE_SANITIZERS=ON -DBUILD_TESTS=ON
cmake --build build-san && ./build-san/file_organizer_tests
```

---

## Communication Guidelines

### When Responding to User
- Be concise and technical
- Focus on implementation, not explanations
- Provide working code, not pseudocode
- Test before claiming completion
- Cite files with `@filepath:line` format

### When Making Changes
- Explain what you're doing briefly
- Show test results after changes
- Update documentation proactively
- Mention any trade-offs or decisions

### When Stuck
- Clearly state the blocker
- Provide context and what you've tried
- Suggest alternatives if possible
- Ask specific questions

---

## Phase-Specific Guidelines

### Phase 1 (Current): Foundation & Testing
- **Focus:** Test coverage, baselines, documentation
- **Quality:** Zero warnings, zero leaks
- **Speed:** Don't optimize prematurely
- **Docs:** Document everything

### Phase 2 (Upcoming): ONNX Integration
- **Focus:** AI inference, embedding generation
- **Quality:** Benchmark inference latency
- **Integration:** Vendor ONNX Runtime properly
- **Testing:** Test with real models

### Phase 3: Polish & Performance
- **Focus:** Optimization, UX improvements
- **Quality:** Profile and optimize hot paths
- **Parallelization:** Thread-safe operations
- **Caching:** Persistent embedding cache

### Phase 4: Release
- **Focus:** Packaging, beta testing
- **Quality:** User documentation
- **Distribution:** Homebrew, AppImage
- **Feedback:** Gather and iterate

---

## Performance Targets

### Current (Debug)
- Scan 10k files: 138ms (~75k files/s)
- Extension lookup: 171ns
- FileInfo construction: 6.9μs

### Target (Release)
- Scan 10k files: <30ms (>300k files/s)
- MIME detection overhead: <10%
- Embedding generation: <50ms per file
- Duplicate detection: <5s for 10k files

---

## Known Issues & Limitations

### Current
- Extension-only categorization (no content analysis)
- No duplicate detection
- No conflict resolution for duplicate filenames
- Single-threaded scanning

### Planned Fixes
- Week 2: MIME detection, robustness improvements
- Week 4-5: Duplicate detection via embeddings
- Week 6: Parallelization, caching
- Week 7: Conflict resolution, UX polish

---

## Resources

### Documentation
- **Project Rules:** `.windsurf/rules.md`
- **Current State:** `docs/PROJECT_STATE.md`
- **MVP Plan:** `.windsurf/plans/file-organizer-mvp-bf8546.md`
- **Testing Guide:** `docs/TESTING.md`
- **Quick Start:** `QUICKSTART.md`
- **Workflow:** `.windsurf/workflows/file-organizer-dev.md`

### External
- [C++20 Reference](https://en.cppreference.com/)
- [Google Test Docs](https://google.github.io/googletest/)
- [Google Benchmark Docs](https://github.com/google/benchmark)
- [FTXUI Examples](https://github.com/ArthurSonzogni/FTXUI)

---

## Success Metrics

### Phase 1 (Current)
- ✅ 42 tests passing
- ✅ CI green on all platforms
- ✅ Zero compiler warnings
- ✅ No memory leaks
- ⏳ MIME detection working

### MVP (Week 8)
- 95%+ duplicate detection accuracy
- <5s scan for 10k files
- 50+ beta testers
- 70%+ recommendation rate
- 10+ GitHub stars

---

## Final Notes

- **Always verify** changes with tests before claiming completion
- **Keep documentation** in sync with code
- **Follow the plan** but adapt when necessary
- **Ask questions** when requirements are unclear
- **Test on both platforms** when possible (macOS + Linux)

**Remember:** Quality over speed. A well-tested, documented feature is better than a rushed implementation.

---

**Last Updated:** 2026-03-26  
**For AI Agents:** Read `.windsurf/rules.md` and `docs/PROJECT_STATE.md` at session start
