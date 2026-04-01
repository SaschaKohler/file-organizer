cmake_minimum_required(VERSION 3.20)
project(file_organizer CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g3 -O0 -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -Wall -Wextra")

option(ENABLE_SANITIZERS "Enable address and undefined behavior sanitizers" OFF)
if(ENABLE_SANITIZERS)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,undefined -fno-omit-frame-pointer")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address,undefined")
endif()

option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_BENCHMARKS "Build benchmarks" ON)
option(ENABLE_ONNX_RUNTIME "Enable ONNX Runtime for embeddings" OFF)

include(FetchContent)

FetchContent_Declare(
  ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
  GIT_TAG v5.0.0
)

FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json
  GIT_TAG v3.11.3
)

FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog
  GIT_TAG v1.12.0
)

FetchContent_Declare(
  eigen
  GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
  GIT_TAG 3.4.0
)

if(BUILD_TESTS)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest
    GIT_TAG v1.14.0
  )
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
endif()

if(BUILD_BENCHMARKS)
  FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark
    GIT_TAG v1.8.3
  )
  set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
endif()

if(BUILD_TESTS AND BUILD_BENCHMARKS)
  FetchContent_MakeAvailable(ftxui json spdlog eigen googletest benchmark)
elseif(BUILD_TESTS)
  FetchContent_MakeAvailable(ftxui json spdlog eigen googletest)
elseif(BUILD_BENCHMARKS)
  FetchContent_MakeAvailable(ftxui json spdlog eigen benchmark)
else()
  FetchContent_MakeAvailable(ftxui json spdlog eigen)
endif()

# Ensure Eigen target is available when fetched from source
if(NOT TARGET Eigen3::Eigen)
  add_library(Eigen3::Eigen INTERFACE IMPORTED)
  target_include_directories(Eigen3::Eigen INTERFACE ${eigen_SOURCE_DIR})
endif()

# Find libmagic (system library)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBMAGIC REQUIRED libmagic)
if(NOT LIBMAGIC_FOUND)
  message(FATAL_ERROR "libmagic not found. Install with: brew install libmagic (macOS) or apt-get install libmagic-dev (Ubuntu)")
endif()

add_executable(file_organizer
  src/main.cpp
  src/file_scanner.cpp
  src/organizer.cpp
  src/config.cpp
  src/ui.cpp
)

target_include_directories(file_organizer PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(file_organizer PRIVATE
  ftxui::screen
  ftxui::dom
  ftxui::component
  nlohmann_json::nlohmann_json
)

# Library for shared code (used by main executable and tests)
add_library(file_organizer_lib STATIC
  src/file_scanner.cpp
  src/organizer.cpp
  src/config.cpp
  src/mime_detector.cpp
  src/logger.cpp
  src/embedding_engine.cpp
  src/vector_ops.cpp
  src/duplicate_detector.cpp
  src/quarantine.cpp
)

target_include_directories(file_organizer_lib PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(ENABLE_ONNX_RUNTIME)
  pkg_check_modules(ONNXRUNTIME libonnxruntime)
  if(NOT ONNXRUNTIME_FOUND)
    set(ONNXRUNTIME_PREFIX "/usr/local/opt/onnxruntime")
    if(EXISTS "${ONNXRUNTIME_PREFIX}/include")
      set(ONNXRUNTIME_INCLUDE_DIRS "${ONNXRUNTIME_PREFIX}/include")
      set(ONNXRUNTIME_LIBRARY_DIRS "${ONNXRUNTIME_PREFIX}/lib")
      set(ONNXRUNTIME_LIBRARIES "onnxruntime")
      set(ONNXRUNTIME_FOUND TRUE)
      message(STATUS "Found ONNX Runtime at: ${ONNXRUNTIME_PREFIX}")
    else()
      message(WARNING "ONNX Runtime not found. Install with: brew install onnxruntime")
    endif()
  endif()
  
  if(ONNXRUNTIME_FOUND)
    target_include_directories(file_organizer_lib PUBLIC ${ONNXRUNTIME_INCLUDE_DIRS})
    target_link_directories(file_organizer_lib PUBLIC ${ONNXRUNTIME_LIBRARY_DIRS})
    target_link_libraries(file_organizer_lib PUBLIC ${ONNXRUNTIME_LIBRARIES})
    target_compile_definitions(file_organizer_lib PUBLIC USE_ONNXRUNTIME)
  endif()
endif()

target_link_libraries(file_organizer_lib PUBLIC
  nlohmann_json::nlohmann_json
  ${LIBMAGIC_LIBRARIES}
  spdlog::spdlog
  Eigen3::Eigen
)

target_include_directories(file_organizer_lib PUBLIC
  ${LIBMAGIC_INCLUDE_DIRS}
)

# Link main executable with library
target_link_libraries(file_organizer PRIVATE file_organizer_lib)

# Tests
if(BUILD_TESTS)
  enable_testing()
  
  add_executable(file_organizer_tests
    tests/file_scanner_test.cpp
    tests/organizer_test.cpp
    tests/config_test.cpp
    tests/mime_detector_test.cpp
    tests/logger_test.cpp
    tests/embedding_engine_test.cpp
    tests/vector_ops_test.cpp
    tests/duplicate_detector_test.cpp
    tests/quarantine_test.cpp
  )
  
  target_link_libraries(file_organizer_tests PRIVATE
    file_organizer_lib
    GTest::gtest_main
  )
  
  include(GoogleTest)
  gtest_add_tests(TARGET file_organizer_tests)
endif()

# Debug tool for duplicate detection
add_executable(test_duplicate_detection
  test_duplicate_detection.cpp
)

target_link_libraries(test_duplicate_detection PRIVATE
  file_organizer_lib
)

# Benchmarks
if(BUILD_BENCHMARKS)
  add_executable(file_organizer_bench
    benchmarks/scan_benchmark.cpp
  )
  
  target_link_libraries(file_organizer_bench PRIVATE
    file_organizer_lib
    benchmark::benchmark_main
  )
endif()
