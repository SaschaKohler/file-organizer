#include "duplicate_detector.hpp"
#include "embedding_engine.hpp"
#include "file_scanner.hpp"
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <directory>\n";
    return 1;
  }

  fs::path test_dir = argv[1];
  
  std::cout << "Scanning directory: " << test_dir << "\n";
  
  FileScanner scanner(test_dir, false, 0);
  auto files = scanner.scan();
  
  std::cout << "Found " << files.size() << " files\n";
  
  for (const auto& file : files) {
    std::cout << "  - " << file.path.filename() << " (" << file.size << " bytes)\n";
  }
  
  EmbeddingEngine engine;
  DuplicateDetector detector(engine, 0.95f);
  
  std::cout << "\nSearching for duplicates...\n";
  std::cout << "ONNX enabled: " << (engine.is_enabled() ? "yes" : "no") << "\n";
  
  auto duplicates = detector.find_duplicates(files);
  
  std::cout << "\nFound " << duplicates.size() << " duplicate groups:\n";
  
  for (size_t i = 0; i < duplicates.size(); ++i) {
    const auto& group = duplicates[i];
    std::cout << "\nGroup " << (i + 1) << " (similarity: " << group.similarity_score << "):\n";
    for (const auto& file : group.files) {
      std::cout << "  - " << file.filename() << "\n";
    }
  }
  
  return 0;
}
