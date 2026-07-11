#pragma once

#include <utility>
#include <vector>

#include "Gomoku.h"

namespace gomoku {

// Owns user-facing game state. The engine is rebuilt from this move list after
// each mutation, keeping undo and transport/UI concerns out of Gomoku itself.
class GameSession {
 public:
  GameSession(const std::vector<int>& patternLookup1, const std::vector<int>& patternLookup2);

  bool placePiece(int x, int y);
  [[nodiscard]] std::pair<int, int> placeComputerMove();
  int undo(int moves = 1);
  void reset();

  [[nodiscard]] Piece getPiece(int x, int y) const;
  [[nodiscard]] Piece currentTurn() const;
  [[nodiscard]] int checkWinner() const;
  [[nodiscard]] bool isBoardFull() const;
  [[nodiscard]] int moveCount() const;

 private:
  struct Move {
    int x;
    int y;
  };

  void rebuild();

  Gomoku game;
  std::vector<Move> moves;
};

}  // namespace gomoku
