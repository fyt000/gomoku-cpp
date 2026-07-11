#include "GameSession.h"

namespace gomoku {

GameSession::GameSession(EvaluationTablesPtr evaluationTables) : game(std::move(evaluationTables))
{
}

bool GameSession::placePiece(int x, int y)
{
  if (game.checkWinner() != Piece::EMPTY || game.isBoardFull() || !game.placePiece(x, y)) {
    return false;
  }
  moves.push_back({x, y});
  return true;
}

std::pair<int, int> GameSession::placeComputerMove(SearchOptions options)
{
  if (game.checkWinner() != Piece::EMPTY || game.isBoardFull()) {
    return {-1, -1};
  }

  const auto move = game.placePiece(options);
  if (move.first < 0 || move.second < 0) {
    return move;
  }
  moves.push_back({move.first, move.second});
  return move;
}

int GameSession::undo(int movesToUndo)
{
  int undone = 0;
  while (undone < movesToUndo && !moves.empty()) {
    moves.pop_back();
    ++undone;
  }
  if (undone) {
    rebuild();
  }
  return undone;
}

void GameSession::reset()
{
  moves.clear();
  rebuild();
}

Piece GameSession::getPiece(int x, int y) const
{
  return game.getPiece(x, y);
}

Piece GameSession::currentTurn() const
{
  return game.currentTurn();
}

Piece GameSession::checkWinner() const
{
  return game.checkWinner();
}

bool GameSession::isBoardFull() const
{
  return game.isBoardFull();
}

int GameSession::moveCount() const
{
  return static_cast<int>(moves.size());
}

void GameSession::rebuild()
{
  game.reset();
  for (const Move move : moves) {
    (void)game.placePiece(move.x, move.y);
  }
}

}  // namespace gomoku
