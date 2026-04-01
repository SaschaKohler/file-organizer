# File Organizer - Terminal GUI

Ein praktisches Tool zum automatischen Organisieren von Dateien mit moderner Terminal-UI.

## Features

- 📁 **Automatische Kategorisierung** nach Dateityp (170+ Dateiformate)
- 🎨 **Moderne Terminal GUI** mit FTXUI
  - Tab-Navigation zwischen Panels
  - Dynamische Terminal-Anpassung
  - Scrollbare Panels mit visuellen Indikatoren
  - Fokus-Hervorhebung (gelbe Borders & Titel)
- 🔍 **Live-Vorschau** der zu organisierenden Dateien
- 🔒 **Dry-Run Mode** zum sicheren Testen
- ⚙️ **JSON-Konfiguration** für flexible Regeln
- 📊 **Statistiken** nach Kategorie
- 🗓️ **Datum-basierte Unterordner** (optional)
- 🔄 **Duplikat-Erkennung** mit Echtzeit-Fortschrittsanzeige
- 🧵 **Thread-safe** asynchrone Verarbeitung
- 📝 **Strukturiertes Logging** mit spdlog
- 🎯 **MIME-basierte Erkennung** mit libmagic

## C++20 Features verwendet

- ✅ **Concepts**: `PathLike`, `Organizable` für Type Constraints
- ✅ **Ranges**: `std::views::filter` für funktionale Operationen
- ✅ **std::filesystem**: Moderne Dateisystem-Operationen
- ✅ **std::optional**: Sichere Rückgabewerte
- ✅ **Template Constraints**: Type-safe generische Funktionen
- ✅ **Structured Bindings**: `auto [category, count]`
- ✅ **std::ranges::sort**: Moderne Sortierung

## Build

### Quick Start
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/file_organizer
```

### Development Build (with tests)
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
cmake --build build --parallel
./build/file_organizer_tests
```

### With Sanitizers
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON
cmake --build build --parallel
```

## Usage

```bash
# Mit Default-Verzeichnis (~/Downloads)
./file_organizer

# Mit spezifischem Verzeichnis
./file_organizer /path/to/directory
```

## Keyboard Controls

### Navigation
- `w` / `Tab` - Wechsel zwischen Panels (FileList, Stats, Preview, CategoryDist)
- `↑/↓` - Navigate durch Dateien (im FileList-Panel)
- `Esc` - Zurück zum Hauptmenü (aus Duplikat-/Browser-Ansicht)

### Aktionen
- `Enter` - Organisiere ausgewählte Datei
- `a` - Organisiere alle Dateien
- `r` - Refresh/Neu scannen
- `d` - Toggle Dry-Run Mode
- `p` - Toggle Preview-Panel
- `f` - Duplikat-Erkennung starten

### Erweitert
- `s` - Sortier-Modus wechseln (Name/Größe/Kategorie/Extension)
- `o` - Sortier-Reihenfolge umkehren
- `b` - Source-Verzeichnis durchsuchen
- `t` - Target-Verzeichnis durchsuchen
- `c` - Kategorien auswählen
- `u` - Letzte Aktion rückgängig machen
- `+/-` - Scan-Tiefe anpassen
- `q` - Quit

## Konfiguration

Die Konfiguration wird in `~/.config/file-organizer/config.json` gespeichert:

```json
{
  "watch_dir": "/Users/you/Downloads",
  "organize_base_dir": "/Users/you/Organized",
  "dry_run": true,
  "rules": [
    {
      "category": "images",
      "target_dir": "Images",
      "create_subdirs_by_date": false
    },
    {
      "category": "documents",
      "target_dir": "Documents",
      "create_subdirs_by_date": true
    }
  ]
}
```

## Unterstützte Kategorien

- **Images**: jpg, png, gif, svg, webp, heic, heif, cr2, nef, arw, dng (RAW), etc.
- **Videos**: mp4, avi, mkv, mov, webm, flv, etc.
- **Audio**: mp3, wav, flac, aac, ogg, m4a, etc.
- **Documents**: pdf, doc, docx, txt, md, rtf, odt, epub, mobi, azw, etc.
- **Spreadsheets**: xls, xlsx, csv, ods, numbers, etc.
- **Presentations**: ppt, pptx, odp, key, etc.
- **Code**: cpp, py, js, java, ts, tsx, jsx, vue, svelte, go, rust, etc.
- **Archives**: zip, rar, 7z, tar, gz, tgz, xz, iso, dmg, etc.
- **Installers**: dmg, pkg, exe, msi, deb, rpm, apk, ipa, appimage, etc.

**Insgesamt 170+ Dateiformate** mit MIME-basierter Erkennung für maximale Genauigkeit.
- **Other**: Alles andere

## Sicherheit

- **Dry-Run Mode** ist standardmäßig aktiviert
- Keine Dateien werden gelöscht, nur verschoben
- Zielverzeichnisse werden automatisch erstellt
- Konflikte werden durch Überschreiben vermieden (TODO)

## Testing

Das Projekt verfügt über umfassende Tests:

- **42 Unit Tests** (Google Test)
- **Performance Benchmarks** (Google Benchmark)
- **CI/CD Pipeline** (GitHub Actions für macOS + Linux)

```bash
# Tests ausführen
./build/file_organizer_tests

# Benchmarks ausführen
./build/file_organizer_bench
```

Siehe `docs/TESTING.md` für Details.

## Development

### Project Status

🚀 **Phase 1 Complete:** Foundation & Testing Infrastructure
- ✅ Comprehensive test suite (42 tests, 100% passing)
- ✅ Performance benchmarks established
- ✅ CI/CD pipeline operational
- ✅ Sanitizer support (ASan, UBSan)

📋 **Next Phase:** AI-Powered Duplicate Detection
- MIME-type detection
- ONNX Runtime integration
- Embedding-based similarity detection

See `docs/PHASE1_SUMMARY.md` and `.windsurf/plans/file-organizer-mvp-bf8546.md` for roadmap.

## Erweiterungen (TODO)

- [ ] Konfliktauflösung bei doppelten Dateinamen
- [ ] Undo-Funktion
- [ ] Custom Rules via UI
- [ ] File Preview in UI
- [ ] Batch-Operationen mit Fortschrittsanzeige
- [ ] Watch-Mode (automatisch bei neuen Dateien)
- [ ] **AI Duplicate Detection** (In Progress)
- [ ] MIME-Type Detection
- [ ] Content-based Classification
