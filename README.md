# Spreadsheet — мини‑таблица с формулами на C++ (ANTLR4)

Этот проект — компактная реализация электронных таблиц на C++17 с поддержкой:
- текстовых и формульных ячеек;
- арифметики, скобок и ссылок на другие ячейки (`A1`, `B2`, …);
- кэширования вычислений и инвалидации зависимых ячеек;
- проверки и предотвращения циклических зависимостей;
- печати таблицы значениями и «сырым» текстом;
- парсинга формул через **ANTLR4** по грамматике `Formula.g4`.

> Репозиторий содержит готовый `antlr4-4.13.2-complete.jar` и модуль `FindANTLR.cmake`, так что кодогенерация лексера/парсера выполняется при сборке CMake.

---

## Содержание
- [Демо/запуск](#демозапуск)
- [Сборка](#сборка)
  - [Зависимости](#зависимости)
  - [Linux / macOS](#linux--macos)
  - [Windows (Visual Studio)](#windows-visual-studio)
  - [Генерация парсера вручную (опционально)](#генерация-парсера-вручную-опционально)
- [Как устроено](#как-устроено)
  - [Публичные интерфейсы](#публичные-интерфейсы)
  - [Кэш и инвалидация](#кэш-и-инвалидация)
  - [Обнаружение циклов](#обнаружение-циклов)
  - [Грамматика формул](#грамматика-формул)
- [Примеры использования](#примеры-использования)
- [Структура проекта](#структура-проекта)
- [Тесты](#тесты)
- [Идеи для развития](#идеи-для-развития)
- [Лицензия](#лицензия)

---

## Демо/запуск

После сборки появляется консольный бинарник **`spreadsheet`**. Он запускает набор автотестов (см. `main.cpp`) и пишет прогресс в `stderr`:

```bash
./build/spreadsheet
# Пример вывода:
# TestSomething OK
# TestFormulaArithmetic OK
# ...
```

Вы можете также подключить библиотеку и использовать API напрямую (см. раздел «Примеры использования»).

---

## Сборка

### Зависимости
- C++ компилятор с поддержкой **C++17** (GCC 9+/Clang 10+/MSVC 2019+).
- **CMake 3.8+**.
- **Java Runtime (JRE 8+)** — нужен для запуска `antlr4-*.jar` при кодогенерации.
  - Если JRE не установлен, поставьте его и убедитесь, что `java` доступна в `PATH`.

> `antlr4-4.13.2-complete.jar` уже лежит в корне проекта. CMake‑модуль `FindANTLR.cmake` постарается найти его автоматически.

### Linux / macOS

```bash
# из корня проекта
cmake -S . -B build
cmake --build build -j
./build/spreadsheet
```

Если CMake не нашёл jar, укажите путь явно:

```bash
cmake -S . -B build -DANTLR_EXECUTABLE=./antlr4-4.13.2-complete.jar
cmake --build build -j
```

### Windows (Visual Studio)

Откройте «x64 Native Tools Command Prompt for VS» и выполните:

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DANTLR_EXECUTABLE=antlr4-4.13.2-complete.jar
cmake --build build --config Release
build\Release\spreadsheet.exe
```

> Для MSVC в `CMakeLists.txt` выставлена сборка со «shared CRT», чтобы GTest внутри зависимостей корректно линкуется.

### Генерация парсера вручную (опционально)

Если вы меняете грамматику `Formula.g4` и хотите сгенерировать файлы сами:

```bash
java -jar antlr4-4.13.2-complete.jar -Dlanguage=Cpp -visitor -no-listener Formula.g4
```

Сгенерированные `.cpp/.h` можно класть в отдельную папку (например, `generated/`) и прописать её в `CMakeLists.txt`. По умолчанию это делает CMake‑цель, описанная в `FindANTLR.cmake`.

---

## Как устроено

### Публичные интерфейсы

Определены в [`common.h`](common.h). Ключевые сущности:

- **`Position {int row, col;}`** — позиция ячейки (индексация с 0).  
  Поддерживает парсинг/печать, сравнение и проверку корректности.

- **`Size {int rows, cols;}`** — видимый размер таблицы.

- **`FormulaError`** — тип ошибки вычисления формулы:
  - `Ref` — некорректная ссылка на ячейку;
  - `Value` — ячейка не приводится к числу;
  - `Arithmetic` — деление на ноль.

- **`CellInterface`** — общая абстракция ячейки:
  - `using Value = std::variant<std::string, double, FormulaError>;`
  - `Value GetValue() const;` — видимое значение (текст/число/ошибка);
  - `std::string GetText() const;` — «сырой» текст (или выражение формулы);
  - `std::vector<Position> GetReferencedCells() const;` — прямые зависимости.

- **`SheetInterface`** — таблица:
  - `void SetCell(Position pos, std::string text);`
  - `const CellInterface* GetCell(Position pos) const;`
  - `CellInterface* GetCell(Position pos);`
  - `void ClearCell(Position pos);`
  - `Size GetPrintableSize() const;`
  - `void PrintValues(std::ostream&) const;`
  - `void PrintTexts(std::ostream&) const;`

- **`std::unique_ptr<SheetInterface> CreateSheet();`** — фабрика пустой таблицы.

Реализации:
- **`Sheet`** (`sheet.h/.cpp`) — хеш‑таблица ячеек, печать и расчёт printable‑области.
- **`Cell`** (`cell.h/.cpp`) — внутренняя стратегия `Impl`:
  - пустая (`EmptyImpl`),
  - текстовая (`TextImpl`),
  - формульная (`FormulaImpl`) с объектом `FormulaInterface`.
- **`FormulaInterface`** (`formula.h/.cpp`) — обёртка над AST/парсером:
  - `Value Evaluate(const SheetInterface&) const;`
  - `std::string GetExpression() const;`
  - `std::vector<Position> GetReferencedCells() const;`

ANTLR‑AST и парсерная логика — в `FormulaAST.h/.cpp` (+ сгенерированные файлы ANTLR при сборке).

### Кэш и инвалидация

`Cell::FormulaImpl` хранит `mutable std::optional<FormulaInterface::Value> cache_`.  
При изменении ячейки:
1. обновляется содержимое (текст/формула);
2. выполняется **проверка циклов**;
3. ячейка **инвалидирует свой кэш** и рекурсивно инвалидирует все зависимые от неё ячейки;
4. вычисление заново произойдёт лениво — при следующем `GetValue()`.

### Обнаружение циклов

При установке формулы выполняется DFS по графу зависимостей (`Cell::CheckCircularDependency`). Если путь ведёт в текущую ячейку — бросается `CircularDependencyException`, а изменение откатывается.

### Грамматика формул

Грамматика ANTLR задаётся в [`Formula.g4`](Formula.g4). Коротко:
```antlr
expr
    : '(' expr ')'              # Parens
    | (ADD | SUB) expr          # UnaryOp
    | expr (MUL | DIV) expr     # BinaryOp
    | expr (ADD | SUB) expr     # BinaryOp
    | CELL                      # Cell (A1, B2, ...)
    | NUMBER                    # Literal
    ;
CELL   : [A-Z]+[0-9]+ ;
NUMBER : UINT EXPONENT? | UINT? '.' UINT EXPONENT? ;
```

---

## Примеры использования

```cpp
#include "common.h"

int main() {
    auto sheet = CreateSheet();

    // Простые значения
    sheet->SetCell({0,0}, "42");         // A1
    sheet->SetCell({0,1}, "'=text");     // B1 — экранированный текст, видимое значение: "=text"

    // Формулы
    sheet->SetCell({1,0}, "=A1*2");      // A2 -> 84
    sheet->SetCell({1,1}, "=A2 + 3");    // B2 -> 87

    // Получение значений
    auto* a2 = sheet->GetCell({1,0});
    if (a2) {
        const auto val = a2->GetValue(); // std::variant<string, double, FormulaError>
        // ... обработка значения ...
    }

    // Печать
    sheet->PrintValues(std::cout); // табличное представление значений (с табами)
    sheet->PrintTexts(std::cout);  // исходный текст ячеек
}
```

Ошибки вычислений представлены `FormulaError` и попадают в `CellInterface::Value`.
Некорректные позиции приводят к `InvalidPositionException`.
Синтаксические ошибки формул — `FormulaException`.
Циклы — `CircularDependencyException`.

---

## Структура проекта

```
.
  CMakeLists.txt
  FindANTLR.cmake
  Formula.g4
  FormulaAST.cpp
  FormulaAST.h
  cell.cpp
  cell.h
  common.h
  formula.cpp
  formula.h
  main.cpp
  sheet.cpp
  sheet.h
  structures.cpp
  test_runner_p.h
```

> В корне также присутствует `antlr4-4.13.2-complete.jar` (исключён из листинга).

---

## Тесты

Автотесты расположены в `main.cpp` и используют лёгкий раннер из `test_runner_p.h`.
Запуская бинарник `spreadsheet`, вы запускаете весь набор тестов.  
Если тест падает, утилита печатает подробное сообщение и ненулевой код возврата.

---

## Идеи для развития

- Больше функций формул (минимум/максимум/сумма/среднее, и т. п.).
- Диапазоны и массивные формулы.
- Сохранение/загрузка таблицы (CSV/TSV, собственный формат).
- Улучшенный форматтер печати (выравнивание, ширина столбцов).
- Профилирование и более умная инвалидация зависимостей (topo‑sort).
- Интеграция с GUI (Qt/ImGui) для наглядного редактирования.
