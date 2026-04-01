# Testing Infrastructure

## Overview

The file-organizer project uses Google Test for unit testing and Google Benchmark for performance testing.

## Test Coverage

**Current Status:** 42 tests, 100% passing

### Test Suites

#### FileScannerTest (16 tests)
- Directory scanning (empty, single, multiple files)
- File categorization by extension
- Category-specific tests (images, videos, audio, documents, code, archives)
- Edge cases (unknown extensions, case-insensitive)
- File metadata (size, modified time)
- Sorting and filtering

#### OrganizerTest (12 tests)
- Dry-run vs live mode
- File movement operations
- Directory creation
- Rule-based organization
- Date-based subdirectories
- Batch operations
- Special characters handling
- Concept constraints

#### ConfigTest (14 tests)
- Default configuration
- JSON serialization/deserialization
- File I/O operations
- Error handling
- Round-trip preservation

## Running Tests

### Build and Run All Tests
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
cmake --build build --parallel
./build/file_organizer_tests
```

### Run Specific Test Suite
```bash
./build/file_organizer_tests --gtest_filter=FileScannerTest.*
```

### Run Single Test
```bash
./build/file_organizer_tests --gtest_filter=FileScannerTest.ScanEmptyDirectory
```

### Verbose Output
```bash
./build/file_organizer_tests --gtest_verbose
```

## Benchmarks

### Running Benchmarks
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_BENCHMARKS=ON
cmake --build build --parallel
./build/file_organizer_bench
```

### Current Performance Baselines (Debug Build)

| Operation | File Count | Time | Throughput |
|-----------|-----------|------|------------|
| Scan Files | 10 | 0.16 ms | 64.7k files/s |
| Scan Files | 100 | 1.19 ms | 86.3k files/s |
| Scan Files | 1,000 | 12.7 ms | 80.9k files/s |
| Scan Files | 10,000 | 138 ms | 75.0k files/s |
| Categorize | 100 | 4.21 μs | - |
| Categorize | 1,000 | 50.2 μs | - |
| Categorize | 10,000 | 423 μs | - |
| Extension Lookup | - | 171 ns | - |
| FileInfo Construction | - | 6.9 μs | - |

**Note:** Release builds will be significantly faster (3-5x improvement expected).

## Sanitizers

### Address Sanitizer (ASan)
Detects memory errors:
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON
cmake --build build
./build/file_organizer_tests
```

### Running with Sanitizer Options
```bash
ASAN_OPTIONS=detect_leaks=1:strict_string_checks=1 ./build/file_organizer_tests
```

## Continuous Integration

GitHub Actions runs tests on every push:
- **Platforms:** macOS-latest, Ubuntu-latest
- **Build Types:** Debug, Release
- **Sanitizers:** Enabled on Ubuntu Debug builds

See `.github/workflows/ci.yml` for configuration.

## Adding New Tests

### Example Test
```cpp
TEST_F(FileScannerTest, NewFeature) {
  create_test_file("test.txt");
  
  FileScanner scanner(test_dir_);
  auto files = scanner.scan();
  
  ASSERT_EQ(files.size(), 1);
  EXPECT_EQ(files[0].category, "documents");
}
```

### Test Fixtures
All test suites use fixtures for setup/teardown:
- `FileScannerTest` - Creates/cleans temp directory
- `OrganizerTest` - Creates source and destination directories
- `ConfigTest` - Manages temp config files

## Coverage Goals

- **Target:** 80%+ code coverage
- **Current:** ~85% (estimated)
- **Tool:** Use `gcov` or `llvm-cov` for detailed coverage reports

## Best Practices

1. **Isolation:** Each test should be independent
2. **Cleanup:** Use fixtures for automatic cleanup
3. **Assertions:** Use `ASSERT_*` for critical checks, `EXPECT_*` for non-critical
4. **Naming:** Descriptive test names (e.g., `HandlesSpecialCharactersInFilename`)
5. **Speed:** Keep tests fast (<100ms per test)

## Troubleshooting

### Tests Fail on CI but Pass Locally
- Check platform-specific behavior (path separators, case sensitivity)
- Verify temp directory permissions
- Check for race conditions in parallel tests

### Slow Tests
- Profile with `--gtest_filter` to isolate
- Check for unnecessary file I/O
- Use smaller test datasets

### Memory Leaks
- Run with ASan enabled
- Check fixture cleanup in `TearDown()`
- Verify RAII patterns
