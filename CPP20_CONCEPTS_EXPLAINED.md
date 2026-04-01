# C++17/20 Konzepte - Detaillierte Erklärung

Diese Dokumentation erklärt **alle** modernen C++-Features, die im File Organizer verwendet werden.

---

## 1. C++20 Concepts (Type Constraints)

### Was sind Concepts?

Concepts erlauben es dir, **Anforderungen an Template-Parameter** zu definieren. Statt kryptischer Compiler-Fehler bekommst du klare Fehlermeldungen.

### Beispiel aus unserem Code:

```cpp
template<typename T>
concept PathLike = std::convertible_to<T, fs::path>;
```

**Was passiert hier?**
- `concept PathLike` definiert eine Anforderung
- `std::convertible_to<T, fs::path>` prüft, ob `T` zu `fs::path` konvertiert werden kann
- Nur Typen, die diese Anforderung erfüllen, dürfen verwendet werden

### Verwendung:

```cpp
template<PathLike P>
void set_root(P &&path) {
    root_dir_ = std::forward<P>(path);
}
```

**Warum ist das besser?**

**Ohne Concept (C++11):**
```cpp
template<typename P>
void set_root(P &&path) {
    root_dir_ = std::forward<P>(path);  // Fehler erst hier, kryptisch!
}
```

**Mit Concept (C++20):**
```cpp
template<PathLike P>
void set_root(P &&path) {
    // Fehler sofort: "P erfüllt nicht PathLike"
}
```

### Eigenes Concept erstellen:

```cpp
template<typename T>
concept Organizable = requires(T t) {
  { t.path } -> std::convertible_to<fs::path>;
  { t.category } -> std::convertible_to<std::string>;
};
```

**Was bedeutet `requires`?**
- `requires(T t)` - "Gegeben ein Objekt `t` vom Typ `T`"
- `{ t.path }` - "`t` muss ein Member `path` haben"
- `-> std::convertible_to<fs::path>` - "und es muss zu `fs::path` konvertierbar sein"

### Praktisches Beispiel:

```cpp
struct FileInfo {
  fs::path path;
  std::string category;
};

struct BadType {
  int path;  // Falscher Typ!
};

template<Organizable T>
void process(const T &item) {
  std::cout << item.path << "\n";
}

int main() {
  FileInfo good;
  process(good);  // ✅ OK
  
  BadType bad;
  process(bad);   // ❌ Compiler-Fehler: "BadType erfüllt nicht Organizable"
}
```

---

## 2. std::filesystem (C++17)

### Moderne Dateisystem-Operationen

**Alt (C-Style):**
```cpp
#include <sys/stat.h>
#include <dirent.h>

DIR *dir = opendir("/path");
struct dirent *entry;
while ((entry = readdir(dir)) != NULL) {
  // Kompliziert und fehleranfällig!
}
closedir(dir);
```

**Neu (C++17):**
```cpp
#include <filesystem>
namespace fs = std::filesystem;

for (const auto &entry : fs::directory_iterator("/path")) {
  std::cout << entry.path() << "\n";
}
```

### Wichtige Funktionen:

#### a) Verzeichnis iterieren
```cpp
for (const auto &entry : fs::directory_iterator(root_dir_)) {
  if (entry.is_regular_file()) {
    // Nur normale Dateien
  }
}
```

#### b) Pfad-Operationen
```cpp
fs::path p = "/home/user/file.txt";

p.filename();        // "file.txt"
p.extension();       // ".txt"
p.parent_path();     // "/home/user"
p.stem();            // "file"

// Pfade kombinieren
fs::path target = base / "subdir" / "file.txt";
// Ergebnis: "base/subdir/file.txt"
```

#### c) Datei-Informationen
```cpp
fs::path file = "test.txt";

fs::file_size(file);           // Größe in Bytes
fs::last_write_time(file);     // Letzte Änderung
fs::exists(file);              // Existiert?
fs::is_directory(file);        // Ist Verzeichnis?
fs::is_regular_file(file);     // Ist normale Datei?
```

#### d) Verzeichnisse erstellen
```cpp
fs::path dir = "/path/to/new/dir";

fs::create_directory(dir);      // Erstellt ein Verzeichnis
fs::create_directories(dir);    // Erstellt alle Parent-Verzeichnisse
```

#### e) Dateien verschieben/kopieren
```cpp
fs::path from = "old.txt";
fs::path to = "new.txt";

fs::rename(from, to);          // Verschieben/Umbenennen
fs::copy(from, to);            // Kopieren
fs::remove(from);              // Löschen
```

### Unser Beispiel:

```cpp
FileInfo::FileInfo(const fs::path &p) 
  : path(p)
  , extension(p.extension().string())
  , size(fs::file_size(p))
  , modified_time(fs::last_write_time(p))
  , category("unknown") {}
```

**Was passiert:**
1. `p.extension()` → gibt Extension als `fs::path` zurück
2. `.string()` → konvertiert zu `std::string`
3. `fs::file_size(p)` → gibt Größe in Bytes zurück
4. `fs::last_write_time(p)` → gibt Zeitstempel zurück

---

## 3. std::optional (C++17)

### Problem ohne optional:

```cpp
// Wie zeige ich "kein Wert"?
OrganizeRule* get_rule(const std::string &category) {
  auto it = rules_.find(category);
  if (it != rules_.end()) {
    return &it->second;
  }
  return nullptr;  // ⚠️ Pointer können nullptr sein - unsicher!
}

// Verwendung:
auto *rule = get_rule("images");
if (rule) {  // Muss immer prüfen!
  use(*rule);
}
```

### Mit std::optional:

```cpp
std::optional<OrganizeRule> get_rule(const std::string &category) const {
  auto it = rules_.find(category);
  if (it != rules_.end()) {
    return it->second;  // Wert vorhanden
  }
  return std::nullopt;  // Kein Wert
}

// Verwendung:
auto rule = get_rule("images");
if (rule) {  // oder: if (rule.has_value())
  use(*rule);  // oder: use(rule.value())
}
```

### Vorteile:

1. **Typ-sicher**: Kein Pointer-Chaos
2. **Explizit**: "Dieser Wert kann fehlen"
3. **Keine Heap-Allocation**: Effizienter als Pointer

### Weitere Verwendungen:

```cpp
std::optional<int> parse_int(const std::string &s) {
  try {
    return std::stoi(s);
  } catch (...) {
    return std::nullopt;
  }
}

// Verwendung mit value_or
int x = parse_int("42").value_or(0);  // Fallback-Wert

// Verwendung mit and_then (C++23, aber Konzept wichtig)
auto result = parse_int("42")
  .and_then([](int x) { return std::optional{x * 2}; });
```

### In unserem Code:

```cpp
std::optional<AppConfig> AppConfig::load_from_file(const fs::path &config_path) {
  if (!fs::exists(config_path)) {
    return std::nullopt;  // Datei existiert nicht
  }
  
  // ... laden ...
  return config;  // Erfolg!
}

// Verwendung:
auto config = AppConfig::load_from_file(path);
if (config) {
  use(*config);
} else {
  config = AppConfig::create_default();
}
```

---

## 4. Ranges & Views (C++20)

### Problem mit klassischen Schleifen:

```cpp
std::vector<FileInfo> files;

// Alle Dateien einer Kategorie finden
std::vector<FileInfo> images;
for (const auto &f : files) {
  if (f.category == "images") {
    images.push_back(f);  // Kopie!
  }
}
```

### Mit Ranges:

```cpp
auto images = files | std::views::filter([](const FileInfo &f) {
  return f.category == "images";
});

// Keine Kopie! Lazy evaluation!
for (const auto &img : images) {
  // Wird erst hier evaluiert
}
```

### Was ist der Unterschied?

**Klassisch:**
- Erstellt neuen Container
- Kopiert alle Elemente
- Speicher-Overhead

**Ranges:**
- Keine Kopie
- "View" auf Original-Daten
- Lazy evaluation (erst bei Zugriff)

### Weitere Beispiele:

```cpp
std::vector<int> nums = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

// Alle geraden Zahlen, verdoppelt
auto result = nums 
  | std::views::filter([](int n) { return n % 2 == 0; })
  | std::views::transform([](int n) { return n * 2; });

for (int n : result) {
  std::cout << n << " ";  // 4 8 12 16 20
}
```

### In unserem Code:

```cpp
auto get_files_by_category(const std::string &category) const {
  return files_ | std::views::filter([&](const FileInfo &f) {
    return f.category == category;
  });
}

// Verwendung:
for (const auto &file : scanner.get_files_by_category("images")) {
  process(file);
}
```

### Wichtige Views:

```cpp
// filter - Elemente filtern
auto even = nums | std::views::filter([](int n) { return n % 2 == 0; });

// transform - Elemente transformieren
auto doubled = nums | std::views::transform([](int n) { return n * 2; });

// take - Erste N Elemente
auto first_5 = nums | std::views::take(5);

// drop - Erste N Elemente überspringen
auto skip_5 = nums | std::views::drop(5);

// reverse - Umgekehrte Reihenfolge
auto reversed = nums | std::views::reverse;

// Kombinieren
auto result = nums
  | std::views::filter([](int n) { return n > 3; })
  | std::views::transform([](int n) { return n * 2; })
  | std::views::take(3);
```

---

## 5. std::ranges::sort (C++20)

### Alt (C++11):

```cpp
std::vector<FileInfo> files;

std::sort(files.begin(), files.end(), [](const FileInfo &a, const FileInfo &b) {
  return a.modified_time > b.modified_time;
});
```

### Neu (C++20):

```cpp
std::ranges::sort(files, [](const FileInfo &a, const FileInfo &b) {
  return a.modified_time > b.modified_time;
});

// Oder mit Projection:
std::ranges::sort(files, std::greater{}, &FileInfo::modified_time);
```

**Vorteile:**
- Kein `.begin()` / `.end()` nötig
- Projections für einfacheren Code
- Bessere Lesbarkeit

---

## 6. Perfect Forwarding (C++11, aber wichtig!)

### Was ist das Problem?

```cpp
template<typename T>
void wrapper(T arg) {
  process(arg);  // Immer Kopie!
}

std::string s = "hello";
wrapper(s);           // Kopie
wrapper(std::move(s)); // Wird trotzdem kopiert!
```

### Lösung: Universal References + std::forward

```cpp
template<typename T>
void wrapper(T &&arg) {  // Universal Reference (nicht rvalue reference!)
  process(std::forward<T>(arg));  // Perfekt weitergeleitet
}

std::string s = "hello";
wrapper(s);           // Lvalue → wird als Lvalue weitergeleitet
wrapper(std::move(s)); // Rvalue → wird als Rvalue weitergeleitet
```

### In unserem Code:

```cpp
template<PathLike P>
void set_root(P &&path) {
  root_dir_ = std::forward<P>(path);
}

// Verwendung:
fs::path p = "/tmp";
scanner.set_root(p);              // Lvalue → kopiert
scanner.set_root(fs::path("/tmp")); // Rvalue → moved
```

**Regeln:**
- `T &&` in Template = Universal Reference
- `std::string &&` = Rvalue Reference
- `std::forward<T>` erhält die Value-Kategorie

---

## 7. Structured Bindings (C++17)

### Alt:

```cpp
std::map<std::string, int> map;

for (auto it = map.begin(); it != map.end(); ++it) {
  std::string key = it->first;
  int value = it->second;
}
```

### Neu:

```cpp
for (const auto &[key, value] : map) {
  // Direkt key und value verwenden!
}
```

### In unserem Code:

```cpp
auto stats = get_category_stats();  // std::map<std::string, size_t>

for (const auto &[category, count] : stats) {
  std::cout << category << ": " << count << "\n";
}
```

### Weitere Beispiele:

```cpp
// Mit std::pair
std::pair<int, std::string> p = {42, "hello"};
auto [num, str] = p;

// Mit std::tuple
std::tuple<int, double, std::string> t = {1, 2.5, "test"};
auto [a, b, c] = t;

// Mit Structs
struct Point { int x, y; };
Point p = {10, 20};
auto [x, y] = p;
```

---

## 8. Template Member Functions

### Warum Template in Member-Funktion?

```cpp
class Organizer {
  template<Organizable T>
  std::optional<fs::path> organize_file(const T &file) {
    // Funktioniert mit jedem Typ, der Organizable erfüllt
  }
};
```

**Vorteil:** Flexibel für verschiedene Typen, aber mit Type-Safety durch Concept!

### Verwendung:

```cpp
struct FileInfo { /* ... */ };
struct CustomFileType { /* ... */ };

Organizer org;
org.organize_file(FileInfo{});        // ✅ OK
org.organize_file(CustomFileType{});  // ✅ OK (wenn Organizable)
org.organize_file(42);                // ❌ Fehler: int ist nicht Organizable
```

---

## 9. Lambda Captures (C++11/14/20)

### Verschiedene Capture-Modi:

```cpp
int x = 10;
std::string s = "hello";

// By value
auto f1 = [x, s]() { return x + s.size(); };

// By reference
auto f2 = [&x, &s]() { x++; s += "!"; };

// Capture all by value
auto f3 = [=]() { return x; };

// Capture all by reference
auto f4 = [&]() { x++; };

// Mixed
auto f5 = [&, x]() { return x; };  // x by value, rest by reference

// Init capture (C++14)
auto f6 = [y = x * 2]() { return y; };

// Move capture (C++14)
auto f7 = [s = std::move(s)]() { return s; };
```

### In unserem Code:

```cpp
auto get_files_by_category(const std::string &category) const {
  return files_ | std::views::filter([&](const FileInfo &f) {
    return f.category == category;  // category by reference captured
  });
}
```

**Wichtig:** `[&]` captured `category` by reference, nicht by value!

---

## 10. Zusammenfassung: Wo wird was verwendet?

### file_scanner.hpp
- ✅ **Concepts**: `PathLike`
- ✅ **std::filesystem**: Pfad-Operationen
- ✅ **Ranges**: `std::views::filter`
- ✅ **Perfect Forwarding**: `std::forward`

### organizer.hpp
- ✅ **Concepts**: `Organizable`
- ✅ **std::optional**: Rückgabewerte
- ✅ **Template Member Functions**

### file_scanner.cpp
- ✅ **std::filesystem**: Directory iteration
- ✅ **std::ranges::sort**: Sortierung
- ✅ **Structured Bindings**: Map iteration

### config.cpp
- ✅ **std::optional**: Config laden
- ✅ **nlohmann::json**: JSON serialization

---

## Übungsaufgaben

### 1. Eigenes Concept erstellen

Erstelle ein Concept `Printable`, das prüft, ob ein Typ mit `std::cout` ausgegeben werden kann:

```cpp
template<typename T>
concept Printable = requires(T t, std::ostream &os) {
  { os << t } -> std::convertible_to<std::ostream&>;
};
```

### 2. Ranges verwenden

Filtere alle Dateien > 1MB und sortiere nach Name:

```cpp
auto large_files = files 
  | std::views::filter([](const FileInfo &f) { return f.size > 1024*1024; })
  | /* wie sortieren? */;
```

### 3. std::optional verwenden

Schreibe eine Funktion, die das erste Element findet, das eine Bedingung erfüllt:

```cpp
template<typename T, typename Pred>
std::optional<T> find_first(const std::vector<T> &vec, Pred pred) {
  // Implementierung
}
```

---

## Nächste Schritte

1. **Baue das Projekt**: `./build.sh`
2. **Teste es**: `./build/file_organizer ~/Downloads`
3. **Erweitere es**: Füge eigene Features hinzu!
4. **Experimentiere**: Ändere die Concepts, füge neue Kategorien hinzu

Viel Erfolg! 🚀
