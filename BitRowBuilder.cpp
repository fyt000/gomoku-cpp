#include "BitRowBuilder.h"

#include <bit>

namespace gomoku {

BitRowBuilder::BitRowBuilder() = default;

BitRowBuilder::BitRowBuilder(std::uint16_t row) : row(row), lenDirty(true) {}

std::uint16_t BitRowBuilder::getRow() const
{
  return row;
}

void BitRowBuilder::reset()
{
  row = kEmptyRow;
  len = 0;
  lenDirty = false;
}

BitRowBuilder& BitRowBuilder::appendCell(bool isSelfStone)
{
  return appendBit(isSelfStone ? 1 : 0);
}

BitRowBuilder& BitRowBuilder::appendBit(int bit)
{
  if (LengthOf(row) >= 15) {
    return *this;
  }
  row = static_cast<std::uint16_t>((row << 1) | (bit & 1));
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
  const int rowLength = length();
  return index >= 0 && index < rowLength &&
         (row & (std::uint16_t{1} << (rowLength - 1 - index))) != 0;
}

bool BitRowBuilder::IsSet(std::uint16_t row, int index)
{
  const int rowLength = LengthOf(row);
  return index >= 0 && index < rowLength &&
         (row & (std::uint16_t{1} << (rowLength - 1 - index))) != 0;
}

std::uint16_t BitRowBuilder::RowSplice(std::uint16_t row, int index)
{
  const int startLen = LengthOf(row);
  if (index < 0 || index > startLen) {
    return row;
  }
  const int remaining = startLen - index;
  const std::uint16_t remainingMask =
      remaining == 0 ? 0 : static_cast<std::uint16_t>((std::uint32_t{1} << remaining) - 1);
  return static_cast<std::uint16_t>((row & remainingMask) | (std::uint16_t{1} << remaining));
}

bool BitRowBuilder::RowStartsWith(std::uint16_t row, std::uint16_t pattern)
{
  const int rowLength = LengthOf(row) + 1;
  const int patternLength = LengthOf(pattern) + 1;
  if (rowLength < patternLength) {
    return false;
  }
  return (row >> (rowLength - patternLength)) == pattern;
}

std::uint16_t BitRowBuilder::GetReverse(std::uint16_t row)
{
  const int rowLength = LengthOf(row);
  std::uint16_t reversedRow = kEmptyRow;
  for (int i = 0; i < rowLength; ++i) {
    reversedRow = static_cast<std::uint16_t>((reversedRow << 1) |
                                             ((row & (std::uint16_t{1} << i)) != 0 ? 1 : 0));
  }
  return reversedRow;
}

int BitRowBuilder::LengthOf(std::uint16_t row)
{
  if (row == 0) {
    return 0;
  }
  return static_cast<int>(std::bit_width(row)) - 1;
}

}  // namespace gomoku
