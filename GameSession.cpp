#include "GameSession.h"

namespace gomoku {

GameSession::GameSession(const std::vector<int>& patternLookup1,
                         const std::vector<int>& patternLookup2)
    : game(patternLookup1, patternLookup2)
{
}

bool GameSession::placePiece(int x, int y)
{
  if (game.checkWinner() || game.isBoardFull() || !game.placePiece(x, y)) {
    return false;
  }
  moves.push_back({x, y});
  rebuild();
  return true;
}

std::pair<int, int> GameSession::placeComputerMove()
{
  if (game.checkWinner() || game.isBoardFull()) {
    return {-1, -1};
  }

  const auto move = game.placePiece();
  if (move.first < 0 || move.second < 0) {
    return move;
  }
  moves.push_back({move.first, move.second});
  rebuild();
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

int GameSession::checkWinner() const
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
