#include "Board.h"

#include <ostream>

namespace gomoku {

Board::Board() = default;

void Board::placePiece(int x, int y, Piece piece)
{
  if (piece < Piece::EMPTY || piece > Piece::WHITE) {
    return;
  }
  board[x][y] = piece;
}

Piece Board::getPiece(int x, int y) const
{
  return board[x][y];
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
      switch (board.board[row][column]) {
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
