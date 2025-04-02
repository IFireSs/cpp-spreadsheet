#pragma once

#include <optional>
#include <set>

#include "common.h"
#include "formula.h"

class Sheet;

class Cell final : public CellInterface {
public:
    Cell(Sheet& sheet);
    ~Cell() override;

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override {
        return impl_->GetReferencedCells();
    }

private:
    void InvalidateCache_rec(bool final); //инвалидация кэша во всех зависимых ячейках
    bool Dfs_rec(Cell* pos/*текущая позиция*/, std::set<Cell*>& black/*пройденные узлы*/, std::set<Cell*>& grey/*текущий список узлов ветви*/); //поиск циклов в глубину

//можете воспользоваться нашей подсказкой, но это необязательно.
    class Impl {
    public:
        virtual ~Impl() = default;

        virtual Value GetValue() const = 0;
        virtual std::string GetText() const = 0;
        virtual std::vector<Position> GetReferencedCells() const {
            return std::vector<Position>();
        };
        virtual std::unique_ptr<Impl> Clone() const = 0;
        virtual void InvalidateCache() {};
    };
    class EmptyImpl final : public Impl {
    public:
        Value GetValue() const override;
        std::string GetText() const override;
        std::vector<Position> GetReferencedCells() const {
            return {};
        }
        std::unique_ptr<Impl> Clone() const override {
            return std::make_unique<EmptyImpl>(*this);
        }
        void InvalidateCache() override {};
    };
    class TextImpl final : public Impl {
    public:
        TextImpl(const std::string& text) : text_(std::move(text)) {
        };
        Value GetValue() const override;
        std::string GetText() const override;
        std::unique_ptr<Impl> Clone() const override {
            return std::make_unique<TextImpl>(*this);
        }
        void InvalidateCache() override {};
    private:
        std::string text_{};
    };
    class FormulaImpl final : public Impl {
        public:
        FormulaImpl(const std::string& text, const SheetInterface& sheet)
        : sheet_interface_(sheet) {
            formula_ = (ParseFormula(text.substr(1)));
        };
        Value GetValue() const override;
        std::string GetText() const override;
        std::vector<Position> GetReferencedCells() const override;
        std::unique_ptr<Impl> Clone() const override {
            return std::make_unique<FormulaImpl>(GetText(), sheet_interface_);
        }
        void InvalidateCache() override; //Инвалидация кэша в данной ячейке
    private:
        std::unique_ptr<FormulaInterface> formula_;
        const SheetInterface& sheet_interface_;
        mutable std::optional<FormulaInterface::Value> cache_{};

    };

    std::unique_ptr<Impl> impl_ = std::make_unique<EmptyImpl>();
    Sheet& sheet_;
};