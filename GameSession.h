#pragma once

#include <utility>
#include <vector>

#include "EvaluationTables.h"
#include "Gomoku.h"

namespace gomoku {

// Owns user-facing game state. Normal moves update the engine directly; undo
// rebuilds it from this compact move history.
class GameSession {
 public:
  explicit GameSession(EvaluationTablesPtr evaluationTables);

  bool placePiece(int x, int y);
  [[nodiscard]] std::pair<int, int> placeComputerMove(SearchOptions options = {});
  int undo(int moves = 1);
  void reset();

  [[nodiscard]] Piece getPiece(int x, int y) const;
  [[nodiscard]] Piece currentTurn() const;
  [[nodiscard]] Piece checkWinner() const;
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
