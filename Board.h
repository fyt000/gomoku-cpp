#pragma once

#include <array>
#include <cstdint>
#include <iosfwd>

namespace gomoku {

inline constexpr int kBoardSize = 15;
inline constexpr int kBoardStride = 16;

enum class Piece : std::uint8_t { EMPTY, BLACK, WHITE };

class Board {
 public:
  using Bitboard = std::array<std::uint64_t, 4>;

  Board();

  template <int Rows, int Columns>
  Board(Piece (&cells)[Rows][Columns])
  {
    static_assert(Rows == kBoardSize && Columns == kBoardSize,
                  "Board dimensions must be 15 by 15.");
    for (int row = 0; row < Rows; ++row) {
      for (int column = 0; column < Columns; ++column) {
        placePiece(row, column, cells[row][column]);
      }
    }
  }

  void placePiece(int x, int y, Piece piece);
  [[nodiscard]] Piece getPiece(int x, int y) const;
  [[nodiscard]] bool isEmpty() const;
  [[nodiscard]] bool isFull() const;
  [[nodiscard]] int pieceCount() const;
  [[nodiscard]] Bitboard occupied() const;
  [[nodiscard]] Bitboard frontier(int radius = 2) const;

  friend std::ostream& operator<<(std::ostream& stream, const Board& board);

 private:
  [[nodiscard]] static bool isValidCoordinate(int x, int y);
  [[nodiscard]] static bool isValidPiece(Piece piece);
  [[nodiscard]] static int paddedIndex(int x, int y);
  [[nodiscard]] static bool isSet(const Bitboard& bits, int index);
  static void set(Bitboard& bits, int index, bool value);
  [[nodiscard]] static Bitboard shiftedLeft(const Bitboard& bits, int amount);
  [[nodiscard]] static Bitboard shiftedRight(const Bitboard& bits, int amount);
  [[nodiscard]] static Bitboard validCells();

  Bitboard black{};
  Bitboard white{};
};

}  // namespace gomoku
