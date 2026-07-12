#include "Board.h"

#include <bit>
#include <ostream>

namespace gomoku {

namespace {

Board::Bitboard bitwiseOr(const Board::Bitboard& lhs, const Board::Bitboard& rhs)
{
  Board::Bitboard result{};
  for (std::size_t i = 0; i < result.size(); ++i) {
    result[i] = lhs[i] | rhs[i];
  }
  return result;
}

void include(Board::Bitboard& destination, const Board::Bitboard& source)
{
  for (std::size_t i = 0; i < destination.size(); ++i) {
    destination[i] |= source[i];
  }
}

}  // namespace

Board::Board() = default;

bool Board::isValidCoordinate(int x, int y)
{
  return x >= 0 && x < kBoardSize && y >= 0 && y < kBoardSize;
}

bool Board::isValidPiece(Piece piece)
{
  return piece == Piece::EMPTY || piece == Piece::BLACK || piece == Piece::WHITE;
}

int Board::paddedIndex(int x, int y)
{
  return x * kBoardStride + y;
}

bool Board::isSet(const Bitboard& bits, int index)
{
  return (bits[static_cast<std::size_t>(index / 64)] & (std::uint64_t{1} << (index % 64))) != 0;
}

void Board::set(Bitboard& bits, int index, bool value)
{
  const std::uint64_t mask = std::uint64_t{1} << (index % 64);
  std::uint64_t& word = bits[static_cast<std::size_t>(index / 64)];
  if (value) {
    word |= mask;
  } else {
    word &= ~mask;
  }
}

Board::Bitboard Board::shiftedLeft(const Bitboard& bits, int amount)
{
  Bitboard result{};
  const int wordShift = amount / 64;
  const int bitShift = amount % 64;
  for (int destination = 3; destination >= 0; --destination) {
    const int source = destination - wordShift;
    if (source < 0) {
      continue;
    }
    result[static_cast<std::size_t>(destination)] |= bits[static_cast<std::size_t>(source)]
                                                     << bitShift;
    if (bitShift != 0 && source > 0) {
      result[static_cast<std::size_t>(destination)] |=
          bits[static_cast<std::size_t>(source - 1)] >> (64 - bitShift);
    }
  }
  return result;
}

Board::Bitboard Board::shiftedRight(const Bitboard& bits, int amount)
{
  Bitboard result{};
  const int wordShift = amount / 64;
  const int bitShift = amount % 64;
  for (int destination = 0; destination < 4; ++destination) {
    const int source = destination + wordShift;
    if (source >= 4) {
      continue;
    }
    result[static_cast<std::size_t>(destination)] |=
        bits[static_cast<std::size_t>(source)] >> bitShift;
    if (bitShift != 0 && source + 1 < 4) {
      result[static_cast<std::size_t>(destination)] |= bits[static_cast<std::size_t>(source + 1)]
                                                       << (64 - bitShift);
    }
  }
  return result;
}

Board::Bitboard Board::validCells()
{
  static const Bitboard valid = [] {
    Bitboard result{};
    for (int row = 0; row < kBoardSize; ++row) {
      for (int column = 0; column < kBoardSize; ++column) {
        const int index = paddedIndex(row, column);
        result[static_cast<std::size_t>(index / 64)] |= std::uint64_t{1} << (index % 64);
      }
    }
    return result;
  }();
  return valid;
}

void Board::placePiece(int x, int y, Piece piece)
{
  if (!isValidCoordinate(x, y) || !isValidPiece(piece)) {
    return;
  }

  const int index = paddedIndex(x, y);
  set(black, index, piece == Piece::BLACK);
  set(white, index, piece == Piece::WHITE);
}

Piece Board::getPiece(int x, int y) const
{
  if (!isValidCoordinate(x, y)) {
    return Piece::EMPTY;
  }

  const int index = paddedIndex(x, y);
  if (isSet(black, index)) {
    return Piece::BLACK;
  }
  if (isSet(white, index)) {
    return Piece::WHITE;
  }
  return Piece::EMPTY;
}

bool Board::isEmpty() const
{
  for (std::size_t i = 0; i < black.size(); ++i) {
    if ((black[i] | white[i]) != 0) {
      return false;
    }
  }
  return true;
}

bool Board::isFull() const
{
  return pieceCount() == kBoardSize * kBoardSize;
}

int Board::pieceCount() const
{
  int count = 0;
  for (std::size_t i = 0; i < black.size(); ++i) {
    count += static_cast<int>(std::popcount(black[i] | white[i]));
  }
  return count;
}

Board::Bitboard Board::occupied() const
{
  return bitwiseOr(black, white);
}

Board::Bitboard Board::frontier(int radius) const
{
  const Bitboard occupiedCells = occupied();
  Bitboard expanded = occupiedCells;
  const Bitboard valid = validCells();

  for (int step = 0; step < radius; ++step) {
    Bitboard neighbours{};
    for (const int direction : {1, 15, 16, 17}) {
      include(neighbours, shiftedLeft(expanded, direction));
      include(neighbours, shiftedRight(expanded, direction));
    }
    include(expanded, neighbours);
    for (std::size_t i = 0; i < expanded.size(); ++i) {
      expanded[i] &= valid[i];
    }
  }

  for (std::size_t i = 0; i < expanded.size(); ++i) {
    expanded[i] &= ~occupiedCells[i];
  }
  return expanded;
}

std::ostream& operator<<(std::ostream& stream, const Board& board)
{
  stream << " ";
  for (int column = 0; column < kBoardSize; ++column) {
    stream.width(3);
    stream << column;
  }
  stream << '\n';

  for (int row = 0; row < kBoardSize; ++row) {
    stream.width(2);
    stream << row;
    for (int column = 0; column < kBoardSize; ++column) {
      switch (board.getPiece(row, column)) {
        case Piece::EMPTY:
          stream << " - ";
          break;
        case Piece::BLACK:
          stream << " X ";
          break;
        case Piece::WHITE:
          stream << " O ";
          break;
      }
    }
    stream << '\n';
  }

  return stream;
}

}  // namespace gomoku
