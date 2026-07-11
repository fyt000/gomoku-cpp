#include "BitRowBuilder.h"

#include <bit>

namespace gomoku {

BitRowBuilder::BitRowBuilder() = default;

BitRowBuilder::BitRowBuilder(int row) : row(row), lenDirty(true) {}

int BitRowBuilder::getRow() const
{
  return row;
}

void BitRowBuilder::reset()
{
  row = kEmptyRow;
  len = 0;
  lenDirty = false;
}

BitRowBuilder& BitRowBuilder::add(bool isEmpty)
{
  return add(isEmpty ? 0 : 1);
}

BitRowBuilder& BitRowBuilder::add(int bit)
{
  row = (row << 1) | (bit & 1);
  lenDirty = true;
  return *this;
}

int BitRowBuilder::length()
{
  if (lenDirty) {
    len = LengthOf(row);
    lenDirty = false;
  }
  return len;
}

void BitRowBuilder::inplaceReverse()
{
  row = GetReverse(row);
  lenDirty = true;
}

bool BitRowBuilder::isSet(int index)
{
  return (row & (1 << (length() - 1 - index))) != 0;
}

bool BitRowBuilder::IsSet(int row, int index)
{
  return (row & (1 << (LengthOf(row) - 1 - index))) != 0;
}

int BitRowBuilder::RowSplice(int row, int index)
{
  const int startLen = LengthOf(row);
  row &= ~(1 << startLen);
  for (int i = 0; i < index; ++i) {
    row &= ~(1 << (startLen - 1 - i));
  }
  row |= 1 << (startLen - index);
  return row;
}

bool BitRowBuilder::RowStartsWith(int row, int pattern)
{
  const int rowLength = LengthOf(row) + 1;
  const int patternLength = LengthOf(pattern) + 1;
  if (rowLength < patternLength) {
    return false;
  }
  return (row >> (rowLength - patternLength)) == pattern;
}

int BitRowBuilder::GetReverse(int row)
{
  const int rowLength = LengthOf(row);
  int reversedRow = kEmptyRow;
  for (int i = 0; i < rowLength; ++i) {
    reversedRow = (reversedRow << 1) | ((row & (1 << i)) != 0 ? 1 : 0);
  }
  return reversedRow;
}

int BitRowBuilder::LengthOf(int row)
{
  if (row == 0) {
    return 0;
  }
  return static_cast<int>(std::bit_width(static_cast<unsigned int>(row))) - 1;
}

}  // namespace gomoku
