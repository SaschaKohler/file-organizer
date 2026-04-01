# Duplicate Detection - Quick Start Guide

## Schnelltest mit generierten Daten

### 1. Test-Duplikate generieren

```bash
cd /Users/saschakohler/Documents/01_Development/Active_Projects/ml-cpp/tasks/file-organizer

# Test-Daten generieren (26 Dateien, 8 Duplikat-Gruppen)
python3 scripts/generate_test_duplicates.py ~/test_duplicates
```

**Erwartete Ausgabe:**
```
✅ Test data generation complete!
Total files created: 26
Expected duplicate groups: 8
```

### 2. Unit-Tests ausführen

```bash
# Alle Duplikat-Tests
./build/file_organizer_tests --gtest_filter='DuplicateDetectorTest.*'

# Erwartetes Ergebnis: 18/18 tests passing
```

### 3. Manuelle Verifikation

```bash
# Duplikate in Test-Verzeichnis finden
./build/file_organizer --scan ~/test_duplicates --find-duplicates

# Erwartetes Ergebnis: 8 Duplikat-Gruppen gefunden
```

---

## Eigene Dateien testen

### Einzelnes Verzeichnis

```bash
# Dein Downloads-Ordner scannen
./build/file_organizer --scan ~/Downloads --find-duplicates
```

### Mehrere Verzeichnisse

```bash
# Dokumente und Bilder scannen
./build/file_organizer --scan ~/Documents --find-duplicates
./build/file_organizer --scan ~/Pictures --find-duplicates
```

---

## Erwartete Ergebnisse

### Mit Test-Daten (~/test_duplicates)

**8 Duplikat-Gruppen sollten gefunden werden:**

| Gruppe | Dateien | Beschreibung |
|--------|---------|--------------|
| 1 | 3 | exact_dup_a_*.txt |
| 2 | 2 | exact_dup_b_*.txt |
| 3 | 2 | large_dup_*.txt |
| 4 | 2 | binary_dup_*.bin |
| 5 | 2 | image_dup_*.ppm |
| 6 | 2 | empty_dup_*.txt |
| 7 | 2 | unicode_dup_*.txt |
| 8 | 3 | same_content.* |

**Nicht als Duplikate erkannt:**
- 5 unique_*.* Dateien
- 3 similar_*.txt Dateien (ähnlich aber nicht identisch)

### Mit eigenen Dateien

**Typische Duplikate:**
- Mehrfach heruntergeladene Dateien (file.pdf, file(1).pdf, file(2).pdf)
- Backup-Kopien (document.txt, document_backup.txt)
- Screenshots mit identischem Inhalt
- Identische Fotos in verschiedenen Ordnern

---

## Troubleshooting

### ❌ Keine Duplikate gefunden (aber erwartet)

**Mögliche Ursachen:**
1. Dateien sind ähnlich, aber nicht byte-identisch
2. Verschiedene Dateigrößen
3. Unterschiedliche Metadaten (bei Bildern)

**Lösung:**
```bash
# Test mit generierten Daten verifizieren
python3 scripts/generate_test_duplicates.py ~/test_duplicates
./build/file_organizer --scan ~/test_duplicates --find-duplicates
```

### ❌ Tests schlagen fehl

**Lösung:**
```bash
# Neu kompilieren
cmake --build build --parallel

# Tests erneut ausführen
./build/file_organizer_tests --gtest_filter='DuplicateDetectorTest.*'
```

### ⚠️ Performance-Probleme

**Bei vielen Dateien (>10k):**
- Hash-basierte Erkennung ist schnell (~2.5k files/s)
- Embedding-basierte Erkennung ist langsam (~20 files/s)

**Empfehlung:**
- Verwende Hash-basierte Erkennung für exakte Duplikate
- Filtere nach Dateityp für Embedding-basierte Erkennung

---

## Nächste Schritte

### Test erfolgreich? ✅

Dann kannst du:
1. Eigene Verzeichnisse scannen
2. Duplikate manuell überprüfen
3. Duplikate löschen/verschieben (in Entwicklung)

### Test fehlgeschlagen? ❌

1. Überprüfe Build: `./build/file_organizer_tests`
2. Überprüfe Test-Daten: `ls -la ~/test_duplicates`
3. Siehe vollständige Dokumentation: `docs/DUPLICATE_DETECTION_TESTING.md`

---

## Weitere Informationen

- **Vollständige Dokumentation:** `docs/DUPLICATE_DETECTION_TESTING.md`
- **Projekt-Status:** `docs/PROJECT_STATE.md`
- **Test-Code:** `tests/duplicate_detector_test.cpp`
- **Implementation:** `src/duplicate_detector.cpp`
