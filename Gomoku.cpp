#include "Gomoku.h"

#include <algorithm>
#include <ostream>

#include "BitRowBuilder.h"

namespace gomoku {

namespace {

constexpr int kSearchDepth = 4;
constexpr int kMateScore = 1'000'000'000;
constexpr int kInfinity = 1'500'000'000;

}  // namespace

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
  if (checkWinner() || isBoardFull()) {
    return {-1, -1};
  }

  if (isBoardEmpty()) {
    constexpr int center = kBoardSize / 2;
    (void)placePiece(center, center);
    return {center, center};
  }

  const SearchBounds bounds = findSearchBounds();
  const auto move = negaMax(kSearchDepth, -kInfinity, kInfinity, turn, -1, -1, bounds);
  int x = std::get<1>(move);
  int y = std::get<2>(move);

  if (x == -1 && y == -1) {
    const auto moves = genBestMoves(turn, bounds);
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

bool Gomoku::isBoardEmpty() const
{
  for (int x = 0; x < kBoardSize; ++x) {
    for (int y = 0; y < kBoardSize; ++y) {
      if (board.getPiece(x, y) != Piece::EMPTY) {
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

int Gomoku::singlePieceEvaluation(int x, int y, Piece player, bool isOddStep)
{
  int value = rowEval(0, y, 1, 0, player, isOddStep);
  value += rowEval(x, 0, 0, 1, player, isOddStep);

  if (y >= x) {
    value += rowEval(0, y - x, 1, 1, player, isOddStep);
  } else {
    value += rowEval(x - y, 0, 1, 1, player, isOddStep);
  }

  if (x + y < kBoardSize) {
    value += rowEval(0, x + y, 1, -1, player, isOddStep);
  } else {
    value += rowEval(x + y - (kBoardSize - 1), kBoardSize - 1, 1, -1, player, isOddStep);
  }
  return value;
}

Piece Gomoku::otherPlayer(Piece piece) const
{
  return piece == Piece::WHITE ? Piece::BLACK : Piece::WHITE;
}

Gomoku::SearchBounds Gomoku::findSearchBounds() const
{
  SearchBounds bounds{.minX = kBoardSize / 2,
                      .maxX = kBoardSize / 2,
                      .minY = kBoardSize / 2,
                      .maxY = kBoardSize / 2};

  for (int x = 0; x < kBoardSize; ++x) {
    for (int y = 0; y < kBoardSize; ++y) {
      if (board.getPiece(x, y) != Piece::EMPTY) {
        bounds = includeMove(bounds, x, y);
      }
    }
  }
  return bounds;
}

Gomoku::SearchBounds Gomoku::includeMove(SearchBounds bounds, int x, int y)
{
  return {.minX = std::min(bounds.minX, x),
          .maxX = std::max(bounds.maxX, x),
          .minY = std::min(bounds.minY, y),
          .maxY = std::max(bounds.maxY, y)};
}

std::vector<Gomoku::ScoreXY> Gomoku::genBestMoves(Piece current, SearchBounds bounds)
{
  const Piece opponent = otherPlayer(current);
  const int minX = std::max(0, bounds.minX - 2);
  const int maxX = std::min(kBoardSize - 1, bounds.maxX + 2);
  const int minY = std::max(0, bounds.minY - 2);
  const int maxY = std::min(kBoardSize - 1, bounds.maxY + 2);

  std::vector<ScoreXY> scores;
  scores.reserve((maxX - minX + 1) * (maxY - minY + 1));

  for (int x = minX; x <= maxX; ++x) {
    for (int y = minY; y <= maxY; ++y) {
      if (board.getPiece(x, y) != Piece::EMPTY) {
        continue;
      }

      const int beforeScore = singlePieceEvaluation(x, y, current, true) +
                              singlePieceEvaluation(x, y, opponent, true);
      board.placePiece(x, y, current);
      if (singlePieceWinner(x, y)) {
        board.placePiece(x, y, Piece::EMPTY);
        return {std::make_tuple(1, x, y)};
      }

      int currentScore = singlePieceEvaluation(x, y, current, true);
      board.placePiece(x, y, opponent);
      currentScore += singlePieceEvaluation(x, y, opponent, true) - beforeScore;
      board.placePiece(x, y, Piece::EMPTY);
      scores.emplace_back(currentScore, x, y);
    }
  }

  std::sort(scores.begin(), scores.end(), [](const ScoreXY& lhs, const ScoreXY& rhs) {
    return std::get<0>(lhs) > std::get<0>(rhs);
  });
  return scores;
}

Gomoku::ScoreXY Gomoku::negaMax(int depth, int alpha, int beta, Piece next, int lastX,
                                 int lastY, SearchBounds bounds)
{
  if (lastX >= 0 && singlePieceWinner(lastX, lastY)) {
    // The player who made the preceding move won, so the side to move has lost.
    // More remaining depth means the loss happened earlier in the searched line.
    return std::make_tuple(-kMateScore - depth, -1, -1);
  }
  if (depth == 0) {
    const Piece opponent = otherPlayer(next);
    return std::make_tuple(evalBoard(next, false) - evalBoard(opponent, true), -1, -1);
  }

  int bestX = -1;
  int bestY = -1;
  int bestValue = -kInfinity;
  for (const auto& score : genBestMoves(next, bounds)) {
    const int x = std::get<1>(score);
    const int y = std::get<2>(score);
    board.placePiece(x, y, next);
    const auto nextScore = negaMax(depth - 1, -beta, -alpha, otherPlayer(next), x, y,
                                   includeMove(bounds, x, y));
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

  constexpr int deltaX[] = {0, 1, 1, 1};
  constexpr int deltaY[] = {1, 0, 1, -1};
  for (int direction = 0; direction < 4; ++direction) {
    int count = 1;
    for (const int sign : {-1, 1}) {
      int nextX = x;
      int nextY = y;
      for (int i = 0; i < 4; ++i) {
        nextX += sign * deltaX[direction];
        nextY += sign * deltaY[direction];
        if (nextX < 0 || nextX >= kBoardSize || nextY < 0 || nextY >= kBoardSize ||
            board.getPiece(nextX, nextY) != piece) {
          break;
        }
        ++count;
      }
    }
    if (count >= 5) {
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
