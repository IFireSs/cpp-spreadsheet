#include "cell.h"
#include "sheet.h"
#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <optional>

// Реализуйте следующие методы

Cell::Cell(Sheet& sheet) :  sheet_(sheet) {
}

Cell::~Cell() = default;

void Cell::Set(std::string text) {
    auto temp_impl = impl_->Clone();
    if (text.empty()) {
        impl_ = std::make_unique<EmptyImpl>();
    } else if (text.size() > 1 && text[0] == FORMULA_SIGN) {
        impl_ = std::make_unique<FormulaImpl>(text, sheet_);
    } else if (!text.empty()) {
        impl_ = std::make_unique<TextImpl>(text);
    }

    std::set<Cell*> black, grey;
    if (Dfs_rec(this, black, grey)) {
        impl_ = std::move(temp_impl);
        throw CircularDependencyException("");
    }
    InvalidateCache_rec(true);
}

void Cell::Clear() {
    impl_.reset();
    InvalidateCache_rec(true);
}

Cell::Value Cell::GetValue() const {
    return impl_->GetValue();
}
std::string Cell::GetText() const {
    return impl_->GetText();
}

Cell::Value Cell::EmptyImpl::GetValue() const {
    return {};
}
std::string Cell::EmptyImpl::GetText() const {
    return {};
}

Cell::Value Cell::TextImpl::GetValue() const {
    if (text_.at(0) == ESCAPE_SIGN) {
        return text_.substr(1);

    }
    return text_;
}

std::string Cell::TextImpl::GetText() const {
    return text_;
}

Cell::Value Cell::FormulaImpl::GetValue() const {
    if (!cache_.has_value()) {
        cache_ = formula_->Evaluate(sheet_interface_);
    }
    try {
        return std::get<double>(cache_.value());
    } catch (...) {
        return std::get<FormulaError>(cache_.value());
    }
}

std::string Cell::FormulaImpl::GetText() const {
    return FORMULA_SIGN + formula_->GetExpression();
}

std::vector<Position> Cell::FormulaImpl::GetReferencedCells() const {
    return formula_->GetReferencedCells();
};

void Cell::FormulaImpl::InvalidateCache() {
    cache_.reset();
}

void Cell::InvalidateCache_rec(bool final) {
    if (!final) {
        impl_->InvalidateCache();
    }
    for (auto cell_pos : GetReferencedCells()) {
        Cell* cell = sheet_.GetPyreCell(cell_pos);
        if (cell) {
            cell->InvalidateCache_rec(false);
        }
    }
}

bool Cell::Dfs_rec(Cell* pos, std::set<Cell*>& black, std::set<Cell*>& grey) {
    if (black.count(pos)) {
        return false;
    }
    if (grey.count(pos)) {
        return true;
    }
    grey.insert(pos);
    auto referenced_cells = pos->GetReferencedCells();
    for (const auto& ref_pos : referenced_cells) {
        Cell* ref_cell = sheet_.GetPyreCell(ref_pos);
        if (!ref_cell) {
            continue;
        }
        if (Dfs_rec(ref_cell, black, grey)) {
            grey.erase(pos);
            return true;
        }
    }
    grey.erase(pos);
    black.insert(pos);
    return false;
}


