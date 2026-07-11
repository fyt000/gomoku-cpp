#pragma once

#include <iosfwd>

namespace gomoku {

inline constexpr int kBoardSize = 15;

enum Piece { EMPTY, BLACK, WHITE };

class Board {
 public:
  Board();

  template <int Rows, int Columns>
  Board(Piece (&cells)[Rows][Columns])
  {
    static_assert(Rows == kBoardSize && Columns == kBoardSize,
                  "Board dimensions must be 15 by 15.");
    for (int row = 0; row < Rows; ++row) {
      for (int column = 0; column < Columns; ++column) {
        board[row][column] = cells[row][column];
      }
    }
  }

  void placePiece(int x, int y, Piece piece);
  [[nodiscard]] Piece getPiece(int x, int y) const;

  friend std::ostream& operator<<(std::ostream& stream, const Board& board);

 private:
  Piece board[kBoardSize][kBoardSize] = {{Piece::EMPTY}};
};

}  // namespace gomoku
