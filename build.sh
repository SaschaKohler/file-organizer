#!/bin/bash

set -e

echo "Building File Organizer..."

cd "$(dirname "$0")"

mkdir -p build
cd build

cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

make -j$(sysctl -n hw.ncpu)

echo ""
echo "✅ Build complete!"
echo "Run with: ./build/file_organizer"
