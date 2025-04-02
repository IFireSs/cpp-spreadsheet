#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() = default;

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("");
    }
    if (cells_.find(pos) == cells_.end()) {
        cells_.emplace(pos, std::make_unique<Cell>(*this));
    }
    cells_.at(pos)->Set(std::move(text));
    for (const auto& ref : cells_.at(pos)->GetReferencedCells()) {
        if (cells_.find(ref) == cells_.end()) {
            cells_.emplace(ref, std::make_unique<Cell>(*this));
        }
    }
}
Cell* Sheet::GetPyreCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("");
    }
    auto it = cells_.find(pos);
    return (it != cells_.end() && it->second) ? it->second.get() : nullptr;
}

const CellInterface* Sheet::GetCell(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("");
    }
    auto it = cells_.find(pos);
    return (it != cells_.end() && it->second) ? it->second.get() : nullptr;
}
CellInterface* Sheet::GetCell(Position pos) {
    return GetPyreCell(pos);
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("");
    }
    if (const auto& cell = cells_.find(pos); cell != cells_.end() && cell->second != nullptr) {
        cell->second.reset();
    }
}

Size Sheet::GetPrintableSize() const {
    int rows = 0, cols = 0;
    for (const auto &[pos, cell] : cells_) {
        if (cell) {
            rows = std::max(rows, pos.row + 1);
            cols = std::max(cols, pos.col + 1);
        }
    }
    return {rows, cols};
}

void Sheet::PrintValues(std::ostream& output) const {
    auto [rows, cols] = GetPrintableSize();
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            if (col > 0) {
                output << "\t";
            }
            if (const auto& it = cells_.find({ row, col }); it != cells_.end() && it->second != nullptr && !it->second->GetText().empty()) {
                std::visit([&](const auto& value) {
                    output << value;
                },
                it->second->GetValue());
            }
        }
        output << "\n";
    }
}
void Sheet::PrintTexts(std::ostream& output) const {
    auto [rows, cols] = GetPrintableSize();
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            if (col > 0) {
                output << "\t";
            }
            if (const auto& it = cells_.find({ row, col }); it != cells_.end() && it->second!= nullptr && !it->second->GetText().empty()) {
                output << it->second->GetText();
            }
        }
        output << "\n";
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}

