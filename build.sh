#!/bin/bash

set -e

echo "Building File Organizer..."

cd "$(dirname "$0")"

mkdir -p build
cd build

cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

cmake --build . --parallel

echo ""
echo "✅ Build complete!"
echo "Run with: ./build/file_organizer"
