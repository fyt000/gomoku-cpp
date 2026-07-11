#include "Gomoku.h"

#include <algorithm>
#include <ostream>

#include "BitRowBuilder.h"

namespace gomoku {

Gomoku::Gomoku() = default;

Gomoku::Gomoku(const std::vector<int>& patternLookup1, const std::vector<int>& patternLookup2)
    : patternLookup1(patternLookup1), patternLookup2(patternLookup2)
{
}

bool Gomoku::placePiece(int x, int y)
{
  if (x < 0 || x >= kBoardSize || y < 0 || y >= kBoardSize ||
      board.getPiece(x, y) != Piece::EMPTY) {
    return false;
  }
  board.placePiece(x, y, turn);
  turn = otherPlayer(turn);
  return true;
}

std::pair<int, int> Gomoku::placePiece()
{
  const auto move = negaMax(4, -99999999, 99999999, turn, turn);
  int x = std::get<1>(move);
  int y = std::get<2>(move);

  if (x == -1 && y == -1) {
    const auto moves = genBestMoves(turn);
    if (moves.empty()) {
      return {-1, -1};
    }
    const auto anyMove = moves.front();
    x = std::get<1>(anyMove);
    y = std::get<2>(anyMove);
  }

  placePiece(x, y);
  return {x, y};
}

void Gomoku::reset()
{
  board = Board{};
  turn = Piece::BLACK;
}

Piece Gomoku::getPiece(int x, int y) const
{
  return board.getPiece(x, y);
}

Piece Gomoku::currentTurn() const
{
  return turn;
}

bool Gomoku::isBoardFull() const
{
  for (int x = 0; x < kBoardSize; ++x) {
    for (int y = 0; y < kBoardSize; ++y) {
      if (board.getPiece(x, y) == Piece::EMPTY) {
        return false;
      }
    }
  }
  return true;
}

int Gomoku::evalBoard(Piece player, bool isOddStep)
{
  int value = 0;

  for (int i = 0; i < kBoardSize; ++i) {
    value += rowEval(0, i, 1, 0, player, isOddStep);
    value += rowEval(i, 0, 0, 1, player, isOddStep);
    value += rowEval(0, i, 1, 1, player, isOddStep);
    value += rowEval(0, i, 1, -1, player, isOddStep);
  }

  for (int i = 1; i < kBoardSize; ++i) {
    value += rowEval(i, 0, 1, 1, player, isOddStep);
    value += rowEval(i, kBoardSize - 1, 1, -1, player, isOddStep);
  }

  return value;
}

int Gomoku::rowEval(int x, int y, int deltaX, int deltaY, Piece self, bool isOddStep)
{
  const Piece opponent = otherPlayer(self);
  BitRowBuilder rowBuilder;
  int value = 0;
  for (int i = 0; i < kBoardSize; ++i) {
    if (x < 0 || x >= kBoardSize || y < 0 || y >= kBoardSize) {
      break;
    }
    const Piece piece = board.getPiece(x, y);
    x += deltaX;
    y += deltaY;
    if (piece == opponent) {
      value += subRowEval(rowBuilder.getRow(), isOddStep);
      rowBuilder.reset();
      continue;
    }
    rowBuilder.add(piece == Piece::EMPTY);
  }
  value += subRowEval(rowBuilder.getRow(), isOddStep);
  return value;
}

int Gomoku::subRowEval(int subRow, bool isOddStep)
{
  if (subRow <= (1 << 5)) {
    return 0;
  }
  return isOddStep ? patternLookup1[subRow] : patternLookup2[subRow];
}

Piece Gomoku::otherPlayer(Piece piece) const
{
  return piece == Piece::WHITE ? Piece::BLACK : Piece::WHITE;
}

std::vector<Gomoku::ScoreXY> Gomoku::genBestMoves(Piece current)
{
  const Piece opponent = otherPlayer(current);
  std::vector<ScoreXY> scores;
  int minX = kBoardSize / 2;
  int maxX = kBoardSize / 2 + 1;
  int minY = kBoardSize / 2;
  int maxY = kBoardSize / 2 + 1;

  for (int x = 0; x < kBoardSize; ++x) {
    for (int y = 0; y < kBoardSize; ++y) {
      if (board.getPiece(x, y) != Piece::EMPTY) {
        minX = std::min(x, minX);
        minY = std::min(y, minY);
        maxX = std::max(x, maxX);
        maxY = std::max(y, maxY);
      }
    }
  }

  minX = std::max(0, minX - 2);
  maxX = std::min(kBoardSize - 1, maxX + 2);
  minY = std::max(0, minY - 2);
  maxY = std::min(kBoardSize - 1, maxY + 2);

  for (int x = minX; x <= maxX; ++x) {
    for (int y = minY; y <= maxY; ++y) {
      if (board.getPiece(x, y) != Piece::EMPTY) {
        continue;
      }

      board.placePiece(x, y, current);
      if (singlePieceWinner(x, y)) {
        return {std::make_tuple(1, x, y)};
      }

      int currentScore = evalBoard(current, true);
      board.placePiece(x, y, opponent);
      currentScore += evalBoard(opponent, true);
      board.placePiece(x, y, Piece::EMPTY);
      scores.emplace_back(currentScore, x, y);
    }
  }

  std::sort(scores.begin(), scores.end(), [](const ScoreXY& lhs, const ScoreXY& rhs) {
    return std::get<0>(lhs) > std::get<0>(rhs);
  });
  return scores;
}

Gomoku::ScoreXY Gomoku::negaMax(int depth, int alpha, int beta, Piece start, Piece next)
{
  const Piece opponent = otherPlayer(start);

  if (checkWinner()) {
    const Piece realOpponent = otherPlayer(next);
    return std::make_tuple(evalBoard(next, true) - evalBoard(realOpponent, true), -1, -1);
  }
  if (depth == 0) {
    return std::make_tuple(evalBoard(start, false) - evalBoard(opponent, true), -1, -1);
  }

  int bestX = -1;
  int bestY = -1;
  int bestValue = -99999999;
  for (const auto& score : genBestMoves(next)) {
    const int x = std::get<1>(score);
    const int y = std::get<2>(score);
    board.placePiece(x, y, next);
    const auto nextScore = negaMax(depth - 1, -beta, -alpha, start, otherPlayer(next));
    const int value = -std::get<0>(nextScore);

    if (value > bestValue) {
      bestX = x;
      bestY = y;
      bestValue = value;
    }
    board.placePiece(x, y, Piece::EMPTY);
    alpha = std::max(alpha, value);
    if (beta <= alpha) {
      break;
    }
  }

  return std::make_tuple(bestValue, bestX, bestY);
}

int Gomoku::checkWinner() const
{
  for (int x = 0; x < kBoardSize; ++x) {
    for (int y = 0; y < kBoardSize; ++y) {
      const int winner = singlePieceWinner(x, y);
      if (winner) {
        return winner;
      }
    }
  }
  return 0;
}

int Gomoku::singlePieceWinner(int x, int y) const
{
  const Piece piece = board.getPiece(x, y);
  if (piece == Piece::EMPTY) {
    return 0;
  }

  constexpr int deltaX[] = {0, 1, 1, 1, 0, -1, -1, -1};
  constexpr int deltaY[] = {-1, -1, 0, 1, 1, 1, 0, -1};
  for (int direction = 0; direction < 8; ++direction) {
    int count = 0;
    int nextX = x;
    int nextY = y;
    for (int i = 0; i < 4; ++i) {
      nextX += deltaX[direction];
      nextY += deltaY[direction];
      if (nextX < 0 || nextX >= kBoardSize || nextY < 0 || nextY >= kBoardSize ||
          board.getPiece(nextX, nextY) != piece) {
        break;
      }
      ++count;
    }
    if (count == 4) {
      return static_cast<int>(piece);
    }
  }
  return 0;
}

std::ostream& operator<<(std::ostream& stream, const Gomoku& gomoku)
{
  stream << "Player " << gomoku.turn << "'s turn\n";
  stream << gomoku.board << '\n';
  return stream;
}

}  // namespace gomoku
