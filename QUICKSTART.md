# File Organizer - Quick Start Guide

## 🚀 Resume Development in 60 Seconds

### 1. Check Current Status
```bash
cd /Users/saschakohler/Documents/01_Development/Active_Projects/ml-cpp/tasks/file-organizer

# Read project state
cat docs/PROJECT_STATE.md

# Check what's next
cat .windsurf/plans/file-organizer-mvp-bf8546.md
```

### 2. Verify Everything Works
```bash
# Clean build
rm -rf build

# Build with tests
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
cmake --build build --parallel

# Run tests (should see: 42 tests passing)
./build/file_organizer_tests
```

### 3. Start Coding
**Current Phase:** Week 2 - MIME Detection & Robustness

**Next Task:** Integrate libmagic for MIME-type detection

---

## 📋 Essential Files to Read

| File | Purpose | When to Read |
|------|---------|--------------|
| `.windsurf/rules.md` | Project context & rules | Every new session |
| `docs/PROJECT_STATE.md` | Current status | Every new session |
| `.windsurf/plans/file-organizer-mvp-bf8546.md` | Full roadmap | Weekly |
| `docs/TESTING.md` | Testing guide | When writing tests |
| `docs/PHASE1_SUMMARY.md` | Week 1 summary | Reference |

---

## 🎯 Current Objectives (Week 2)

### Priority 1: MIME Detection
```bash
# Add libmagic to CMakeLists.txt
# Create include/mime_detector.hpp
# Create src/mime_detector.cpp
# Create tests/mime_detector_test.cpp
```

### Priority 2: Robustness
- Handle symlinks
- Permission errors
- Large files (>2GB)
- Unicode filenames

### Priority 3: Logging
- Integrate spdlog
- Add log levels
- Configure rotation

---

## 🔧 Common Commands

### Build
```bash
# Debug build
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
# All tests
./build/file_organizer_tests

# Specific suite
./build/file_organizer_tests --gtest_filter=FileScannerTest.*

# Single test
./build/file_organizer_tests --gtest_filter=FileScannerTest.ScanEmptyDirectory

# Benchmarks
./build/file_organizer_bench
```

### Run Application
```bash
# Default (~/Downloads)
./build/file_organizer

# Specific directory
./build/file_organizer /path/to/directory
```

---

## 📊 Current Status

**Phase:** 1 of 4 (Foundation & Testing)  
**Week:** 1 of 8 ✅ → Week 2 ⏳  
**Tests:** 42/42 passing ✅  
**CI:** All platforms green ✅  
**Performance:** 75k files/s (Debug)

---

## 🐛 Troubleshooting

### Build fails?
```bash
rm -rf build
cmake --version  # Need 3.20+
c++ --version    # Check compiler
```

### Tests fail?
```bash
# Verbose output
./build/file_organizer_tests --gtest_verbose

# With sanitizers
cmake -S . -B build -DENABLE_SANITIZERS=ON
cmake --build build
./build/file_organizer_tests
```

### Need help?
1. Check `docs/TESTING.md`
2. Check `docs/PROJECT_STATE.md`
3. Review `.windsurf/rules.md`

---

## 📈 Progress Tracker

```
Week 1: ████████████████████ 100% ✅ COMPLETE
Week 2: ░░░░░░░░░░░░░░░░░░░░   0% ⏳ STARTING
Week 3: ░░░░░░░░░░░░░░░░░░░░   0%
Week 4: ░░░░░░░░░░░░░░░░░░░░   0%
Week 5: ░░░░░░░░░░░░░░░░░░░░   0%
Week 6: ░░░░░░░░░░░░░░░░░░░░   0%
Week 7: ░░░░░░░░░░░░░░░░░░░░   0%
Week 8: ░░░░░░░░░░░░░░░░░░░░   0%
```

**MVP Progress:** 12.5% (1/8 weeks)

---

## 🎓 Learning Resources

- `LEARNING_PATH.md` - C++20 tutorial
- `CPP20_CONCEPTS_EXPLAINED.md` - Concepts deep dive
- `docs/TESTING.md` - Testing best practices

---

**Ready to code!** 🚀
