#pragma once
#include <iostream>

const int BOARDSIZE = 15;

enum Piece { EMPTY, BLACK, WHITE };

class Board {
public:
  Board();
  template <int R, int C> Board(Piece (&board)[R][C]) {
    for (int i = 0; i < R; i++) {
      for (int j = 0; j < C; j++) {
        this->board[i][j] = board[i][j];
      }
    }
  }
  void placePiece(int x, int y, Piece p);
  Piece getPiece(int x, int y);
  friend std::ostream &operator<<(std::ostream &stream, const Board &gomoku);

private:
  // TODO implement zobristHashing and transposition
  int zobristHash;
  bool hashDirty = true;
  // maybe store it in a way that is locality friendly
  // optimization:
  // each row = 1 single int, 3 types = 2 bits, 15 pieces per row = 30 bits
  // store rows of all directions
  Piece board[BOARDSIZE][BOARDSIZE] = {{Piece::EMPTY}};
};
