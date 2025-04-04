#include <algorithm>

#include "common.h"

#include <cctype>
#include <iostream>
#include <sstream>
#include <cmath>

const int LETTERS = 26;
const int MAX_POSITION_LENGTH = 17;
const int MAX_POS_LETTER_COUNT = 3;

const Position Position::NONE = {-1, -1};

bool Position::operator==(const Position rhs) const {
  return col == rhs.col && row == rhs.row;
}

bool Position::operator<(const Position rhs) const {
  return col < rhs.col || (col == rhs.col && row < rhs.row);
}

bool Position::IsValid() const {
  return col >= 0 && row >= 0 && col < MAX_COLS && row < MAX_ROWS;
}

std::string Position::ToString() const {
  if (!IsValid()) {
    return "";
  }
  std::string str;

  for (int i = col; i>=0; i=i/LETTERS-1){
    str.insert(str.begin(), static_cast<char>(i%LETTERS+65));
  }

  str+=std::to_string(row + 1);

  return str;
}

Position Position::FromString(std::string_view str) {
  Position result{0, 0};
  std::string s(str);
  auto digits = std::find_if(s.begin(), s.end(), [](const char c){return std::isdigit(c);});
  int begin_to_digits = std::distance(s.begin(), digits);
  int digits_to_end = std::distance(digits, s.end());

  if (digits==s.end() || std::any_of(digits, s.end(), [](const char c){return !std::isdigit(c);}) || digits_to_end>5 || begin_to_digits>MAX_POS_LETTER_COUNT) {
    return NONE;
  }
  result.row = std::stoi(s.substr(begin_to_digits, digits_to_end))-1;
  for (int i = 0; i < begin_to_digits; i++) {
    if (std::isupper(str[i])) {
      result.col = result.col * LETTERS + (str[i] - 'A' + 1);
    }
    else {
      return NONE;
    }
  }
  result.col-=1;
  if (!result.IsValid()) {
    return NONE;
  }
  return result;
}

bool Size::operator==(const Size rhs) const {
  return cols == rhs.cols && rows == rhs.rows;
}
