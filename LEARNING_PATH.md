# Learning Path - Schritt für Schritt zum eigenen Tool

Diese Anleitung zeigt dir, wie du **selbst** ein ähnliches Tool von Grund auf implementieren kannst.

---

## Phase 1: Grundlagen - FileScanner (30 min)

### Ziel
Verstehe `std::filesystem` und erstelle einen einfachen File-Scanner.

### Aufgabe 1.1: Dateien auflisten

```cpp
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

int main() {
    fs::path dir = "/Users/you/Downloads";  // Anpassen!
    
    for (const auto &entry : fs::directory_iterator(dir)) {
        std::cout << entry.path().filename() << "\n";
    }
    
    return 0;
}
```

**Kompilieren:**
```bash
clang++ -std=c++17 scanner.cpp -o scanner
./scanner
```

**Fragen zum Verstehen:**
- Was ist `fs::directory_iterator`?
- Was ist der Unterschied zu `fs::recursive_directory_iterator`?
- Was macht `.filename()`?

### Aufgabe 1.2: Datei-Informationen

Erweitere den Code um Dateigrößen:

```cpp
for (const auto &entry : fs::directory_iterator(dir)) {
    if (entry.is_regular_file()) {
        auto size = fs::file_size(entry.path());
        std::cout << entry.path().filename() 
                  << " - " << size << " bytes\n";
    }
}
```

**Zusatz:** Zeige Größe in KB/MB an!

### Aufgabe 1.3: Extension-Filter

Zeige nur `.pdf` Dateien:

```cpp
for (const auto &entry : fs::directory_iterator(dir)) {
    if (entry.is_regular_file() && 
        entry.path().extension() == ".pdf") {
        std::cout << entry.path().filename() << "\n";
    }
}
```

**Challenge:** Mache es case-insensitive (`.PDF` und `.pdf`)!

---

## Phase 2: Concepts verstehen (20 min)

### Aufgabe 2.1: Erstes Concept

```cpp
#include <concepts>
#include <string>

// Definiere ein Concept für "hat einen Namen"
template<typename T>
concept HasName = requires(T t) {
    { t.name } -> std::convertible_to<std::string>;
};

// Teste es
struct Person {
    std::string name;
};

struct Animal {
    int id;  // Kein name!
};

template<HasName T>
void print_name(const T &obj) {
    std::cout << obj.name << "\n";
}

int main() {
    Person p{"Alice"};
    print_name(p);  // ✅ OK
    
    Animal a{42};
    // print_name(a);  // ❌ Compiler-Fehler: Animal erfüllt nicht HasName
}
```

**Fragen:**
- Was passiert, wenn du `print_name(a)` auskommentierst?
- Wie würdest du ein Concept `HasID` definieren?

### Aufgabe 2.2: Mehrere Anforderungen

```cpp
template<typename T>
concept Drawable = requires(T t) {
    { t.x } -> std::convertible_to<int>;
    { t.y } -> std::convertible_to<int>;
    { t.draw() } -> std::same_as<void>;
};

struct Circle {
    int x, y;
    void draw() { std::cout << "Drawing circle\n"; }
};

template<Drawable T>
void render(const T &shape) {
    shape.draw();
}
```

**Challenge:** Erstelle ein Concept `Serializable`, das prüft, ob ein Typ eine `to_json()` Methode hat!

---

## Phase 3: std::optional (15 min)

### Aufgabe 3.1: Sicheres Suchen

```cpp
#include <optional>
#include <vector>
#include <string>

std::optional<std::string> find_by_id(
    const std::vector<std::string> &items, 
    size_t id) {
    
    if (id < items.size()) {
        return items[id];
    }
    return std::nullopt;
}

int main() {
    std::vector<std::string> names = {"Alice", "Bob", "Charlie"};
    
    auto result = find_by_id(names, 1);
    if (result) {
        std::cout << "Found: " << *result << "\n";
    } else {
        std::cout << "Not found\n";
    }
    
    // Mit value_or
    std::string name = find_by_id(names, 99).value_or("Unknown");
    std::cout << name << "\n";  // "Unknown"
}
```

**Challenge:** Schreibe eine Funktion `find_first_matching`, die das erste Element zurückgibt, das eine Bedingung erfüllt!

### Aufgabe 3.2: Config laden

```cpp
#include <optional>
#include <fstream>
#include <filesystem>

struct Config {
    std::string path;
    int max_size;
};

std::optional<Config> load_config(const fs::path &file) {
    if (!fs::exists(file)) {
        return std::nullopt;
    }
    
    std::ifstream f(file);
    if (!f.is_open()) {
        return std::nullopt;
    }
    
    Config cfg;
    f >> cfg.path >> cfg.max_size;
    return cfg;
}

int main() {
    auto cfg = load_config("config.txt");
    if (cfg) {
        std::cout << "Path: " << cfg->path << "\n";
    } else {
        std::cout << "Using defaults\n";
    }
}
```

---

## Phase 4: Ranges & Views (25 min)

### Aufgabe 4.1: Filter

```cpp
#include <ranges>
#include <vector>
#include <iostream>

int main() {
    std::vector<int> nums = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
    // Alle geraden Zahlen
    auto even = nums | std::views::filter([](int n) { 
        return n % 2 == 0; 
    });
    
    for (int n : even) {
        std::cout << n << " ";  // 2 4 6 8 10
    }
}
```

**Wichtig:** `even` ist **keine Kopie**, sondern ein View!

### Aufgabe 4.2: Transform

```cpp
auto doubled = nums | std::views::transform([](int n) { 
    return n * 2; 
});

for (int n : doubled) {
    std::cout << n << " ";  // 2 4 6 8 10 12 14 16 18 20
}
```

### Aufgabe 4.3: Kombinieren

```cpp
// Gerade Zahlen, verdoppelt, erste 3
auto result = nums
    | std::views::filter([](int n) { return n % 2 == 0; })
    | std::views::transform([](int n) { return n * 2; })
    | std::views::take(3);

for (int n : result) {
    std::cout << n << " ";  // 4 8 12
}
```

**Challenge:** Finde alle Dateien > 1MB, sortiere nach Größe, nimm die ersten 5!

### Aufgabe 4.4: Mit eigenen Typen

```cpp
struct File {
    std::string name;
    size_t size;
};

std::vector<File> files = {
    {"a.txt", 100},
    {"b.pdf", 2000},
    {"c.jpg", 500},
};

auto large = files 
    | std::views::filter([](const File &f) { return f.size > 200; });

for (const auto &f : large) {
    std::cout << f.name << "\n";  // b.pdf, c.jpg
}
```

---

## Phase 5: Eigenes Mini-Tool (60 min)

### Projekt: Simple File Categorizer

**Ziel:** Erstelle ein Tool, das Dateien nach Extension kategorisiert.

#### Schritt 1: Header erstellen

```cpp
// categorizer.hpp
#pragma once
#include <filesystem>
#include <string>
#include <map>

namespace fs = std::filesystem;

class FileCategorizer {
public:
    FileCategorizer(fs::path root);
    
    void scan();
    void print_stats() const;
    
private:
    fs::path root_;
    std::map<std::string, size_t> categories_;
    
    std::string get_category(const fs::path &file) const;
};
```

#### Schritt 2: Implementierung

```cpp
// categorizer.cpp
#include "categorizer.hpp"
#include <iostream>

FileCategorizer::FileCategorizer(fs::path root) 
    : root_(std::move(root)) {}

std::string FileCategorizer::get_category(const fs::path &file) const {
    std::string ext = file.extension().string();
    
    if (ext == ".jpg" || ext == ".png") return "images";
    if (ext == ".pdf" || ext == ".txt") return "documents";
    if (ext == ".mp3" || ext == ".wav") return "audio";
    
    return "other";
}

void FileCategorizer::scan() {
    categories_.clear();
    
    for (const auto &entry : fs::directory_iterator(root_)) {
        if (entry.is_regular_file()) {
            std::string cat = get_category(entry.path());
            categories_[cat]++;
        }
    }
}

void FileCategorizer::print_stats() const {
    for (const auto &[category, count] : categories_) {
        std::cout << category << ": " << count << " files\n";
    }
}
```

#### Schritt 3: Main

```cpp
// main.cpp
#include "categorizer.hpp"
#include <iostream>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <directory>\n";
        return 1;
    }
    
    FileCategorizer cat(argv[1]);
    cat.scan();
    cat.print_stats();
    
    return 0;
}
```

#### Schritt 4: Kompilieren

```bash
clang++ -std=c++17 categorizer.cpp main.cpp -o categorizer
./categorizer ~/Downloads
```

### Erweiterungen (mach mindestens 2!)

1. **Concepts hinzufügen**
   - Erstelle ein `PathLike` Concept
   - Verwende es im Konstruktor

2. **Ranges verwenden**
   - Füge eine Methode `get_files_by_category()` hinzu
   - Nutze `std::views::filter`

3. **std::optional**
   - Methode `find_largest_file()` → `std::optional<fs::path>`

4. **Sortierung**
   - Sortiere Kategorien nach Anzahl
   - Nutze `std::ranges::sort`

5. **JSON Export**
   - Speichere Statistiken als JSON
   - Nutze nlohmann/json

---

## Phase 6: Verstehe den File Organizer Code

Jetzt, wo du die Grundlagen kennst, gehe durch den File Organizer Code:

### Checklist

- [ ] Verstehe ich alle Concepts? (`PathLike`, `Organizable`)
- [ ] Kann ich erklären, was `std::views::filter` macht?
- [ ] Verstehe ich `std::optional` in `get_rule()`?
- [ ] Kann ich `std::forward` erklären?
- [ ] Verstehe ich Structured Bindings in `for (const auto &[key, value])`?
- [ ] Kann ich `std::ranges::sort` erklären?

### Übung: Erweitere den File Organizer

1. **Neue Kategorie hinzufügen**
   - Füge "ebooks" hinzu (.epub, .mobi)
   - Teste es!

2. **Neues Concept**
   - Erstelle `Filterable` Concept
   - Nutze es für eine Filter-Funktion

3. **Statistik-Feature**
   - Zeige Gesamt-Speicherplatz pro Kategorie
   - Nutze Ranges für die Berechnung

---

## Debugging-Tipps

### Compiler-Fehler verstehen

**Concept-Fehler:**
```
error: 'BadType' does not satisfy 'Organizable'
```
→ Prüfe, ob `BadType` alle Requirements erfüllt!

**Filesystem-Fehler:**
```
error: no matching function for call to 'file_size'
```
→ Prüfe, ob Datei existiert: `fs::exists(path)`

**Range-Fehler:**
```
error: no match for 'operator|'
```
→ Hast du `#include <ranges>` vergessen?

### Häufige Fehler

1. **Vergessen, `fs::exists()` zu prüfen**
   ```cpp
   // ❌ Falsch
   auto size = fs::file_size(path);
   
   // ✅ Richtig
   if (fs::exists(path)) {
       auto size = fs::file_size(path);
   }
   ```

2. **View zu früh materialisieren**
   ```cpp
   // ❌ Ineffizient
   std::vector<int> filtered;
   for (auto n : nums | std::views::filter(...)) {
       filtered.push_back(n);
   }
   
   // ✅ Besser
   auto filtered = nums | std::views::filter(...);
   // Nutze filtered direkt!
   ```

3. **std::optional falsch verwenden**
   ```cpp
   // ❌ Crash wenn nullopt!
   auto result = find(...);
   std::cout << *result;
   
   // ✅ Richtig
   if (result) {
       std::cout << *result;
   }
   ```

---

## Nächste Schritte

1. **Arbeite die Phasen durch** (1-5)
2. **Baue dein eigenes Tool** (Phase 5)
3. **Studiere den File Organizer** (Phase 6)
4. **Erweitere ihn** mit eigenen Features!

## Ressourcen

- [cppreference.com](https://en.cppreference.com/) - Die Bibel für C++
- [C++20 Ranges Tutorial](https://www.modernescpp.com/index.php/c-20-ranges-library/)
- [Concepts in C++20](https://www.modernescpp.com/index.php/c-20-concepts-the-details/)

Viel Erfolg beim Lernen! 🎓
