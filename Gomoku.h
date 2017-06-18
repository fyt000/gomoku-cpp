#pragma once
#include "Board.h"
#include "TTEntry.h"
#include <iostream>
#include <mutex>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <vector>
#include <atomic>

// not implementing score/weight lookup...
// will add the other script that does it
class Gomoku {
  typedef std::tuple<int, int, int> ScoreXY;

public:
  Gomoku();
  Gomoku(const std::vector<int> &patternLookup1,
         const std::vector<int> &patternLookup2);

  template <int R, int C> void setBoard(Piece (&board)[R][C]) {
    // c++11 is good
    this->curBoard = Board(board);

    // pass in the turn value.
    int pieceCount = 0;
    for (int i = 0; i < 15; i++) {
      for (int j = 0; j < 15; j++) {
        auto p = board[i][j];
        switch (p) {
        case Piece::WHITE:
          pieceCount -= 1;
          break;
        case Piece::BLACK:
          pieceCount += 1;
          break;
        }
      }
    }
    if (pieceCount < 0) {
      turn = Piece::BLACK;
      std::cerr << "black going" << std::endl;
    } else {
      turn = Piece::WHITE;
      std::cerr << "white going" << std::endl;
    }
    // std::cerr<< turn<<std::endl;
  }

  bool placePiece(int x, int y);
  std::pair<int, int> placePiece(int depth = 4);
  int checkWinner();

  friend std::ostream &operator<<(std::ostream &stream, const Gomoku &gomoku);

private:
  int maxScore = 0;
  int wonScore;
  Piece turn = Piece::BLACK;
  Board curBoard;
  const std::vector<int> patternLookup1;
  const std::vector<int> patternLookup2;

  //shared across all!
  //lets see how it works
  // static std::mutex ttLock;
  // static std::unordered_map<Board, TTEntry, BoardHasher> transposition;
  // use boost read/write lock?
  std::mutex ttLock;
  std::unordered_map<Board, TTEntry, BoardHasher> transposition;

  std::atomic<int> nodesVisited;


  int checkWinner(Board &board);
  int evalBoard(Board &board, Piece player, bool isOddStep);
  int rowEval(Board &board, int sx, int sy, int dx, int dy, Piece pType,
              bool isOddStep);

  int subRowEval(int subRow, bool isOddStep) {
    // less than 5
    if (subRow <= (1 << 5))
      return 0;

    if (isOddStep) {
      return patternLookup1[subRow];
    } else {
      return patternLookup2[subRow];
    }
  }

  Piece otherPlayer(Piece p) {
    return p == Piece::WHITE ? Piece::BLACK : Piece::WHITE;
  }

  std::vector<ScoreXY> genBestMoves(Board &board, Piece cur);
  ScoreXY negaScout(Board &board, int depth, int alpha, int beta, Piece start,
                  Piece next);
  int singlePieceWinner(Board &board, int x, int y);

  //
  int singlePieceEvaluation(Board &board, int x, int y, Piece player);

  bool inbound(int x, int y) {
    if (x >= 0 && x < 15 && y >= 0 && y < 15)
      return true;
    return false;
  }
};
