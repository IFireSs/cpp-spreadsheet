#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <iostream>
#include <sstream>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << "#ARITHM!";
}

namespace {
    class Formula : public FormulaInterface {
    public:
        // Реализуйте следующие методы:
        explicit Formula(std::string expression) try : ast_(ParseFormulaAST(expression)){
        }catch (const FormulaException& formulaException) {
            throw formulaException;
        }
        Value Evaluate(const SheetInterface& sheet) const override {
            try {
                return ast_.Execute(sheet);
            }
            catch (FormulaError& formulaError) {
                return formulaError;
            }

        }
        std::string GetExpression() const override {
            std::stringstream ss;
            ast_.PrintFormula(ss);
            return ss.str();
        }
        std::vector<Position> GetReferencedCells() const override {
            std::forward_list<Position> cells = ast_.GetCells();
            std::set<Position> result{cells.begin(), cells.end()};
            return {result.begin(), result.end()};
        }
    private:
        FormulaAST ast_;
    };
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    try {
        return std::make_unique<Formula>(std::move(expression));
    }
    catch (...) {
        throw FormulaException("");
    }
}

FormulaError::FormulaError(Category category)
    : category_(category) {}

FormulaError::Category FormulaError::GetCategory() const {
    return category_;
}

bool FormulaError::operator==(FormulaError rhs) const {
    return category_ == rhs.category_;
}