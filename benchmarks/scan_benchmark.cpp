#include "file_scanner.hpp"
#include <benchmark/benchmark.h>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

class ScanFixture : public benchmark::Fixture {
public:
  fs::path test_dir_;
  
  void SetUp(const ::benchmark::State& state) override {
    test_dir_ = fs::temp_directory_path() / "file_organizer_bench";
    fs::create_directories(test_dir_);
    
    int num_files = state.range(0);
    for (int i = 0; i < num_files; ++i) {
      std::string filename = "file_" + std::to_string(i);
      
      if (i % 10 == 0) filename += ".jpg";
      else if (i % 10 == 1) filename += ".pdf";
      else if (i % 10 == 2) filename += ".mp3";
      else if (i % 10 == 3) filename += ".mp4";
      else if (i % 10 == 4) filename += ".zip";
      else if (i % 10 == 5) filename += ".cpp";
      else if (i % 10 == 6) filename += ".txt";
      else if (i % 10 == 7) filename += ".png";
      else if (i % 10 == 8) filename += ".docx";
      else filename += ".xlsx";
      
      std::ofstream file(test_dir_ / filename);
      file << "benchmark test content";
    }
  }
  
  void TearDown(const ::benchmark::State&) override {
    if (fs::exists(test_dir_)) {
      fs::remove_all(test_dir_);
    }
  }
};

BENCHMARK_DEFINE_F(ScanFixture, ScanFiles)(benchmark::State& state) {
  for (auto _ : state) {
    FileScanner scanner(test_dir_);
    auto files = scanner.scan();
    benchmark::DoNotOptimize(files);
  }
  
  state.SetItemsProcessed(state.iterations() * state.range(0));
}

BENCHMARK_REGISTER_F(ScanFixture, ScanFiles)
  ->Arg(10)
  ->Arg(100)
  ->Arg(1000)
  ->Arg(10000)
  ->Unit(benchmark::kMillisecond);

BENCHMARK_DEFINE_F(ScanFixture, CategorizeFiles)(benchmark::State& state) {
  FileScanner scanner(test_dir_);
  scanner.scan();
  
  for (auto _ : state) {
    auto images = scanner.get_files_by_category("images");
    int count = 0;
    for (const auto& file : images) {
      benchmark::DoNotOptimize(file);
      count++;
    }
    benchmark::DoNotOptimize(count);
  }
}

BENCHMARK_REGISTER_F(ScanFixture, CategorizeFiles)
  ->Arg(100)
  ->Arg(1000)
  ->Arg(10000)
  ->Unit(benchmark::kMicrosecond);

static void BM_ExtensionLookup(benchmark::State& state) {
  fs::path test_path = "/test/file.jpg";
  
  for (auto _ : state) {
    std::string ext = test_path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    benchmark::DoNotOptimize(ext);
  }
}
BENCHMARK(BM_ExtensionLookup);

static void BM_FileInfoConstruction(benchmark::State& state) {
  fs::path temp_file = fs::temp_directory_path() / "bench_test.txt";
  std::ofstream(temp_file) << "test";
  
  for (auto _ : state) {
    FileInfo info(temp_file);
    benchmark::DoNotOptimize(info);
  }
  
  fs::remove(temp_file);
}
BENCHMARK(BM_FileInfoConstruction);
