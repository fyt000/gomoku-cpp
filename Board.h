#pragma once
#include <iostream>
#include <random>


const int BOARDSIZE = 15;

enum Piece { EMPTY, BLACK, WHITE };


class Board {
public:
  Board();
  template <int R, int C> Board(Piece (&board)[R][C]) {
    for (int i = 0; i < R; i++) {
      for (int j = 0; j < C; j++) {
        Piece p = board[i][j];
        zobristHash ^= zobristValue[i][j][(int)p];
        this->board[i][j] = board[i][j];
      }
    }
  }
  // void placePiece(int x, int y, Piece p);
  void placePiece(int x, int y, Piece p, bool rehash = true);
  void undoPiece(int x, int y, bool rehash = true);
  Piece getPiece(int x, int y);
  int64_t getHash() const;

  bool operator==(const Board &other) const;

  friend std::ostream &operator<<(std::ostream &stream, const Board &gomoku);

private:
  // TODO implement zobristHashing and transposition
  int64_t zobristHash = 0;
  // maybe store it in a way that is locality friendly
  // optimization:
  // each row = 1 single int, 3 types = 2 bits, 15 pieces per row = 30 bits
  // store rows of all directions
  Piece board[BOARDSIZE][BOARDSIZE] = {{Piece::EMPTY}};

  static int64_t zobristValue[BOARDSIZE][BOARDSIZE][3];
  static class init {
  public:
    init() {
      //std::mt19937_64 randGen(std::random_device{}());
      //randGen.discard(700000);
      std::mt19937_64 randGen(1233211);
      for (int i = 0; i < BOARDSIZE; i++) {
        for (int j = 0; j < BOARDSIZE; j++) {
          zobristValue[i][j][0] = 0;
          for (int k = 1; k <= 2; k++) {
            zobristValue[i][j][k] = randGen();
          }
        }
      }
    }
  } initializer;
};


class BoardHasher {
  public:
  size_t operator() (Board const& b) const {
    //std::cerr<<b.getHash()<<std::endl;
    return b.getHash();
  }
};

