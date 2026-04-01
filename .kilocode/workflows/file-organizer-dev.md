---
description: File Organizer development workflow and session startup
---

# File Organizer Development Workflow

## Session Startup Checklist

When starting a new development session:

1. **Read Project Context**
   ```bash
   # Essential files to review
   cat .windsurf/rules.md              # Project rules & context
   cat docs/PROJECT_STATE.md           # Current status
   cat QUICKSTART.md                   # Quick commands
   ```

2. **Verify Build Status**
   ```bash
   cd /Users/saschakohler/Documents/01_Development/Active_Projects/ml-cpp/tasks/file-organizer
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
   cmake --build build --parallel
   ./build/file_organizer_tests
   ```

3. **Check Current Phase**
   - Review `.windsurf/plans/file-organizer-mvp-bf8546.md`
   - Identify current week and tasks
   - Update plan if needed

## Development Workflow

### 1. Feature Development (TDD)

```bash
# 1. Write test first
vim tests/new_feature_test.cpp

# 2. Build and verify test fails
cmake --build build
./build/file_organizer_tests --gtest_filter=NewFeatureTest.*

# 3. Implement feature
vim src/new_feature.cpp

# 4. Build and verify test passes
cmake --build build
./build/file_organizer_tests --gtest_filter=NewFeatureTest.*

# 5. Run all tests
./build/file_organizer_tests

# 6. Add benchmark if performance-critical
vim benchmarks/new_feature_benchmark.cpp
./build/file_organizer_bench
```

### 2. Bug Fixing

```bash
# 1. Reproduce with test
vim tests/bug_reproduction_test.cpp

# 2. Build with sanitizers
cmake -S . -B build -DENABLE_SANITIZERS=ON -DBUILD_TESTS=ON
cmake --build build

# 3. Run failing test
./build/file_organizer_tests --gtest_filter=BugTest.*

# 4. Fix bug
vim src/buggy_file.cpp

# 5. Verify fix
./build/file_organizer_tests
```

### 3. Performance Optimization

```bash
# 1. Establish baseline
./build/file_organizer_bench > baseline.txt

# 2. Make optimization
vim src/optimized_file.cpp

# 3. Rebuild Release
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# 4. Compare
./build/file_organizer_bench > optimized.txt
diff baseline.txt optimized.txt

# 5. Document improvement in commit
```

## Code Review Checklist

Before committing:

- [ ] All tests pass: `./build/file_organizer_tests`
- [ ] No compiler warnings: `-Wall -Wextra -Wpedantic`
- [ ] Sanitizers clean: Build with `-DENABLE_SANITIZERS=ON`
- [ ] Benchmarks run (if performance-critical)
- [ ] Documentation updated
- [ ] CHANGELOG.md updated
- [ ] CI will pass (test locally on macOS/Linux if possible)

## Documentation Updates

When adding features:

1. Update `README.md` if user-facing
2. Update `docs/TESTING.md` if adding tests
3. Update `docs/PROJECT_STATE.md` with current status
4. Update `CHANGELOG.md` with changes
5. Update `.windsurf/rules.md` if architecture changes

## Common Development Patterns

### Adding a New Component

```cpp
// 1. Create header: include/new_component.hpp
#pragma once
#include <filesystem>

class NewComponent {
public:
  explicit NewComponent(/* params */);
  void do_something();
private:
  // implementation
};

// 2. Create implementation: src/new_component.cpp
#include "new_component.hpp"

NewComponent::NewComponent(/* params */) {
  // constructor
}

void NewComponent::do_something() {
  // implementation
}

// 3. Create tests: tests/new_component_test.cpp
#include "new_component.hpp"
#include <gtest/gtest.h>

TEST(NewComponentTest, BasicFunctionality) {
  NewComponent comp(/* params */);
  comp.do_something();
  // assertions
}

// 4. Update CMakeLists.txt
// Add to file_organizer_lib sources
// Add to test sources
```

### Adding a Benchmark

```cpp
// benchmarks/new_benchmark.cpp
#include "component.hpp"
#include <benchmark/benchmark.h>

static void BM_NewOperation(benchmark::State& state) {
  Component comp;
  for (auto _ : state) {
    comp.operation();
    benchmark::DoNotOptimize(comp);
  }
}
BENCHMARK(BM_NewOperation);
```

## Testing Strategies

### Unit Tests
- Test each public method
- Test edge cases
- Test error conditions
- Use fixtures for setup/teardown

### Integration Tests
- Test component interactions
- Test full workflows
- Use realistic data

### Performance Tests
- Benchmark hot paths
- Test with various data sizes
- Compare before/after optimizations

## Debugging Tips

### GDB/LLDB
```bash
# Build with debug symbols
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Debug test
lldb ./build/file_organizer_tests
(lldb) run --gtest_filter=FailingTest.*
(lldb) bt  # backtrace
(lldb) frame variable  # inspect variables
```

### Sanitizers
```bash
# Address Sanitizer (memory errors)
cmake -S . -B build -DENABLE_SANITIZERS=ON
cmake --build build
ASAN_OPTIONS=detect_leaks=1 ./build/file_organizer_tests

# Undefined Behavior Sanitizer
UBSAN_OPTIONS=print_stacktrace=1 ./build/file_organizer_tests
```

### Valgrind (Linux)
```bash
valgrind --leak-check=full ./build/file_organizer_tests
```

## CI/CD Workflow

### Local Pre-Push Checks
```bash
# Test on both configurations
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
cmake --build build-debug
./build-debug/file_organizer_tests

cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
cmake --build build-release
./build-release/file_organizer_tests
./build-release/file_organizer_bench

# Test with sanitizers
cmake -S . -B build-san -DENABLE_SANITIZERS=ON -DBUILD_TESTS=ON
cmake --build build-san
./build-san/file_organizer_tests
```

### GitHub Actions
- Automatically runs on push/PR
- Tests on macOS + Ubuntu
- Tests Debug + Release
- Runs sanitizers on Ubuntu
- Runs benchmarks on Release

## Phase-Specific Guidelines

### Phase 1 (Current): Foundation
- Focus on test coverage
- Establish performance baselines
- Document everything
- Keep code simple and readable

### Phase 2 (Upcoming): ONNX Integration
- Vendor ONNX Runtime properly
- Test inference thoroughly
- Benchmark embedding generation
- Cache embeddings efficiently

### Phase 3: Polish
- Optimize hot paths
- Improve error messages
- Add user documentation
- Performance tuning

### Phase 4: Release
- Package for distribution
- Beta testing
- Gather feedback
- Fix critical bugs

## Quick Reference

### Build Configurations
```bash
# Debug (default)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug

# Release (optimized)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# With tests
cmake -S . -B build -DBUILD_TESTS=ON

# With benchmarks
cmake -S . -B build -DBUILD_BENCHMARKS=ON

# With sanitizers
cmake -S . -B build -DENABLE_SANITIZERS=ON

# All options
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_TESTS=ON \
  -DBUILD_BENCHMARKS=ON \
  -DENABLE_SANITIZERS=ON
```

### Test Filters
```bash
# All tests
./build/file_organizer_tests

# Specific suite
./build/file_organizer_tests --gtest_filter=FileScannerTest.*

# Single test
./build/file_organizer_tests --gtest_filter=FileScannerTest.ScanEmptyDirectory

# Exclude tests
./build/file_organizer_tests --gtest_filter=-SlowTest.*

# Verbose
./build/file_organizer_tests --gtest_verbose
```

### Benchmark Options
```bash
# All benchmarks
./build/file_organizer_bench

# Specific benchmark
./build/file_organizer_bench --benchmark_filter=ScanFiles

# Minimum time
./build/file_organizer_bench --benchmark_min_time=1.0s

# Output to file
./build/file_organizer_bench --benchmark_out=results.json
```

## Session End Checklist

Before ending session:

1. [ ] Commit changes with descriptive message
2. [ ] Update `docs/PROJECT_STATE.md` if significant progress
3. [ ] Update plan if priorities changed
4. [ ] Push to remote (if applicable)
5. [ ] Note any blockers or next steps

## Emergency Recovery

If build is broken:
```bash
# Nuclear option
rm -rf build
git status  # check for uncommitted changes
git diff    # review changes

# Rebuild from scratch
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
cmake --build build --parallel
```

If tests are failing:
```bash
# Run with sanitizers to find memory issues
cmake -S . -B build -DENABLE_SANITIZERS=ON -DBUILD_TESTS=ON
cmake --build build
./build/file_organizer_tests

# Run specific failing test with verbose output
./build/file_organizer_tests --gtest_filter=FailingTest.* --gtest_verbose
```

## Resources

- **Project Rules:** `.windsurf/rules.md`
- **Current State:** `docs/PROJECT_STATE.md`
- **MVP Plan:** `.windsurf/plans/file-organizer-mvp-bf8546.md`
- **Testing Guide:** `docs/TESTING.md`
- **Quick Start:** `QUICKSTART.md`
- **Main Roadmap:** `.windsurf/workflows/roadmap-rules.md`
