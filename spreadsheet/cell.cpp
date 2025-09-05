#include "cell.h"

#include <algorithm>

#include "sheet.h"

#include <memory>
#include <string>
#include <optional>

Cell::Cell(Sheet& sheet) :  sheet_(sheet) {
}

Cell::~Cell() = default;

void Cell::Set(std::string text, const Position pos/*нужно исключительно для проверки на самоссылку в этой же ячейке*/) {
    std::unique_ptr<Impl> temp_impl;
    if (text.empty()) {
        temp_impl = std::make_unique<EmptyImpl>();
    } else if (text.size() > 1 && text[0] == FORMULA_SIGN) {
        temp_impl = std::make_unique<FormulaImpl>(text, sheet_);
    } else if (!text.empty()) {
        temp_impl = std::make_unique<TextImpl>(text);
    }
    if (CheckCircularDependency(temp_impl.get(), pos)) {
        throw CircularDependencyException("");
    }
    impl_ = std::move(temp_impl);
    InvalidateCache_rec(true);
}

void Cell::Clear() {
    Set("");
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

bool Cell::CheckCircularDependency(Impl* impl, Position pos) {
    auto ref_positions = impl->GetReferencedCells();
    return std::any_of(ref_positions.begin(), ref_positions.end(),[&](const Position ref_pos) {
        const Cell* ref_cell = sheet_.GetPyreCell(ref_pos);
        return ref_pos == pos || (ref_cell && CheckCircularDependency(ref_cell->impl_.get(), pos));
    });
}
