#!/usr/bin/env python3
"""
Generate test files with duplicates for testing the duplicate detection feature.

Usage:
    python3 generate_test_duplicates.py [output_dir]

This script creates:
- Exact duplicates (identical content)
- Near duplicates (similar content)
- Unique files
- Different file types (text, images, binary)
"""

import os
import sys
import random
import hashlib
from pathlib import Path

def create_text_file(path: Path, content: str):
    """Create a text file with given content."""
    with open(path, 'w', encoding='utf-8') as f:
        f.write(content)

def create_binary_file(path: Path, size: int, seed: int = None):
    """Create a binary file with random or seeded content."""
    if seed is not None:
        random.seed(seed)
    with open(path, 'wb') as f:
        f.write(bytes([random.randint(0, 255) for _ in range(size)]))

def create_image_placeholder(path: Path, width: int, height: int, color: tuple):
    """Create a simple PPM image file (no external dependencies)."""
    with open(path, 'wb') as f:
        # PPM header
        f.write(f"P6\n{width} {height}\n255\n".encode())
        # Pixel data
        pixel = bytes(color)
        for _ in range(width * height):
            f.write(pixel)

def generate_test_duplicates(output_dir: Path):
    """Generate comprehensive test duplicate files."""
    output_dir.mkdir(parents=True, exist_ok=True)
    
    print(f"📁 Generating test files in: {output_dir}")
    
    # 1. Exact text duplicates (Group A)
    print("\n1️⃣  Creating exact text duplicates (Group A)...")
    content_a = "This is an exact duplicate.\nIt has multiple lines.\nAnd some content.\n"
    for i in range(1, 4):
        create_text_file(output_dir / f"exact_dup_a_{i}.txt", content_a)
    print(f"   ✅ Created 3 exact duplicates (Group A)")
    
    # 2. Exact text duplicates (Group B)
    print("\n2️⃣  Creating exact text duplicates (Group B)...")
    content_b = "Another group of duplicates.\nDifferent content from Group A.\n"
    for i in range(1, 3):
        create_text_file(output_dir / f"exact_dup_b_{i}.txt", content_b)
    print(f"   ✅ Created 2 exact duplicates (Group B)")
    
    # 3. Large file duplicates
    print("\n3️⃣  Creating large file duplicates...")
    large_content = "X" * 50000 + "\n" + "Y" * 50000
    for i in range(1, 3):
        create_text_file(output_dir / f"large_dup_{i}.txt", large_content)
    print(f"   ✅ Created 2 large file duplicates (~100KB each)")
    
    # 4. Binary file duplicates
    print("\n4️⃣  Creating binary file duplicates...")
    for i in range(1, 3):
        create_binary_file(output_dir / f"binary_dup_{i}.bin", 1024, seed=42)
    print(f"   ✅ Created 2 binary file duplicates (1KB each)")
    
    # 5. Image duplicates (simple PPM format)
    print("\n5️⃣  Creating image duplicates...")
    for i in range(1, 3):
        create_image_placeholder(output_dir / f"image_dup_{i}.ppm", 100, 100, (255, 0, 0))
    print(f"   ✅ Created 2 image duplicates (100x100 red)")
    
    # 6. Empty file duplicates
    print("\n6️⃣  Creating empty file duplicates...")
    for i in range(1, 3):
        (output_dir / f"empty_dup_{i}.txt").touch()
    print(f"   ✅ Created 2 empty file duplicates")
    
    # 7. Unicode content duplicates
    print("\n7️⃣  Creating Unicode content duplicates...")
    unicode_content = "Hello 世界 🌍\nMultilingual content\nПривет мир\n"
    for i in range(1, 3):
        create_text_file(output_dir / f"unicode_dup_{i}.txt", unicode_content)
    print(f"   ✅ Created 2 Unicode file duplicates")
    
    # 8. Unique files (no duplicates)
    print("\n8️⃣  Creating unique files...")
    unique_files = [
        ("unique_1.txt", "This file is unique.\n"),
        ("unique_2.txt", "Another unique file with different content.\n"),
        ("unique_3.txt", "Yet another unique file.\n"),
        ("unique_4.md", "# Markdown File\n\nUnique markdown content.\n"),
        ("unique_5.json", '{"key": "value", "number": 42}\n'),
    ]
    for filename, content in unique_files:
        create_text_file(output_dir / filename, content)
    print(f"   ✅ Created {len(unique_files)} unique files")
    
    # 9. Similar but not identical files
    print("\n9️⃣  Creating similar (but not identical) files...")
    base_content = "This is the base content.\n"
    create_text_file(output_dir / "similar_1.txt", base_content + "Extra line 1.\n")
    create_text_file(output_dir / "similar_2.txt", base_content + "Extra line 2.\n")
    create_text_file(output_dir / "similar_3.txt", base_content + "Extra line 3.\n")
    print(f"   ✅ Created 3 similar (but not identical) files")
    
    # 10. Different file types with same content
    print("\n🔟 Creating same content in different extensions...")
    same_content = "Same content, different extensions.\n"
    create_text_file(output_dir / "same_content.txt", same_content)
    create_text_file(output_dir / "same_content.md", same_content)
    create_text_file(output_dir / "same_content.log", same_content)
    print(f"   ✅ Created 3 files with same content, different extensions")
    
    # Generate summary
    print("\n" + "="*60)
    print("📊 SUMMARY")
    print("="*60)
    
    all_files = list(output_dir.glob("*"))
    print(f"Total files created: {len(all_files)}")
    
    # Count expected duplicate groups
    duplicate_groups = {
        "Group A (exact)": 3,
        "Group B (exact)": 2,
        "Large files": 2,
        "Binary files": 2,
        "Images": 2,
        "Empty files": 2,
        "Unicode files": 2,
        "Same content (different ext)": 3,
    }
    
    print(f"\nExpected duplicate groups: {len(duplicate_groups)}")
    for group, count in duplicate_groups.items():
        print(f"  • {group}: {count} files")
    
    print(f"\nUnique files: {len(unique_files)}")
    print(f"Similar (not identical): 3")
    
    # Calculate file sizes
    total_size = sum(f.stat().st_size for f in all_files)
    print(f"\nTotal size: {total_size:,} bytes ({total_size / 1024:.1f} KB)")
    
    print("\n✅ Test data generation complete!")
    print(f"\n💡 To test duplicate detection:")
    print(f"   ./build/file_organizer --scan {output_dir} --find-duplicates")

def main():
    if len(sys.argv) > 1:
        output_dir = Path(sys.argv[1])
    else:
        output_dir = Path.home() / "test_duplicates"
    
    print("🔍 Duplicate Detection Test Data Generator")
    print("="*60)
    
    if output_dir.exists():
        response = input(f"\n⚠️  Directory {output_dir} already exists. Overwrite? [y/N]: ")
        if response.lower() != 'y':
            print("❌ Aborted.")
            return
        
        # Clean up existing files
        for f in output_dir.glob("*"):
            if f.is_file():
                f.unlink()
    
    generate_test_duplicates(output_dir)
    
    print(f"\n📂 Test files location: {output_dir.absolute()}")

if __name__ == "__main__":
    main()
