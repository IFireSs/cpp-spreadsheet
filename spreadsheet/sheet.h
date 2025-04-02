#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <unordered_map>
#include <set>

class Sheet : public SheetInterface {
public:
	~Sheet() override;

	void SetCell(Position pos, std::string text) override;

	Cell* GetPyreCell(Position pos);
	const CellInterface* GetCell(Position pos) const override;
	CellInterface* GetCell(Position pos) override;

	void ClearCell(Position pos) override;

	Size GetPrintableSize() const override;

	void PrintValues(std::ostream& output) const override;
	void PrintTexts(std::ostream& output) const override;

	// Можете дополнить ваш класс нужными полями и методами

private:
	// Можете дополнить ваш класс нужными полями и методами
	class Hasher {
	public:
		size_t operator()(const Position pos) const {
			return std::hash<int>()(pos.col) * 37 + std::hash<int>()(pos.row);
		}
	};

	std::unordered_map<Position, std::unique_ptr<Cell>, Hasher> cells_;
};