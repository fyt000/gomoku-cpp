#include "Board.h"
#include <cstring>

Board::Board() {}

int64_t Board::zobristValue[15][15][3];
Board::init Board::initializer;

void Board::placePiece(int x, int y, Piece p, bool rehash) {
  if (p < 0 || p > 2)
    return;
  board[x][y] = p;
  if (rehash) {
    zobristHash ^= zobristValue[x][y][(int)p];
  }
}

void Board::undoPiece(int x, int y, bool rehash) {
  Piece p = getPiece(x, y);
  if (p == Piece::EMPTY) {
    return;
  }
  if (rehash) {
    zobristHash ^= zobristValue[x][y][(unsigned char)p];
  }
  board[x][y] = Piece::EMPTY;
}

Piece Board::getPiece(int x, int y) { return board[x][y]; }

int64_t Board::getHash() const { return zobristHash; }

bool Board::operator==(const Board &other) const {
  return memcmp(board, other.board, sizeof(board)) == 0;
}

// I wish writing an interface could be this simple
std::ostream &operator<<(std::ostream &stream, const Board &board) {
  stream << " ";
  for (int j = 0; j < BOARDSIZE; j++) {
    stream.width(3);
    stream << j;
  }
  stream << std::endl;
  for (int i = 0; i < BOARDSIZE; i++) {
    stream.width(2);
    stream << i;
    for (int j = 0; j < BOARDSIZE; j++) {
      switch (board.board[i][j]) {
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
    stream << std::endl;
  }

  return stream;
}
