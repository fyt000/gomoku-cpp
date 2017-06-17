#include "Gomoku.h"
#include "BitRowBuilder.h"
#include <algorithm>

Gomoku::Gomoku() {}

Gomoku::Gomoku(const std::vector<int> &patternLookup1,
               const std::vector<int> &patternLookup2)
    : patternLookup1(patternLookup1), patternLookup2(patternLookup2) {
  // maxScore =
  // (*std::max_element(patternLookup1.begin(),patternLookup1.end()));
  // maxScore = std::max(maxScore,
  // (*std::max_element(patternLookup2.begin(),patternLookup2.end())));
  // wonScore = 5*maxScore;
}

bool Gomoku::placePiece(int x, int y) {
  if (board.getPiece(x, y) != Piece::EMPTY)
    return false;
  board.placePiece(x, y, turn);
  turn = otherPlayer(turn);
  return true;
}

std::pair<int, int> Gomoku::placePiece() {
  // auto p = alphaBeta(4, -99999999, 99999999, true, turn);
  auto p = negaMax(4, -99999999, 99999999, turn, turn);
  int x = std::get<1>(p);
  int y = std::get<2>(p);
  // lost already
  if (x == -1 && y == -1) {
    auto anyP = genBestMoves(turn)[0];
    x = std::get<1>(anyP);
    y = std::get<2>(anyP);
  }
  placePiece(x, y);
  return {x, y};
}

int Gomoku::evalBoard(Piece player, bool isOddStep) {
  int val = 0;

  // for better locality
  // store these directions each in an array
  // but also find a mapping from placing a piece x,y
  // to update the board

  for (int i = 0; i < BOARDSIZE; i++) {
    // vertical
    val += rowEval(0, i, 1, 0, player, isOddStep);
    // horizontal
    val += rowEval(i, 0, 0, 1, player, isOddStep);

    // diagonal '\'
    val += rowEval(0, i, 1, 1, player, isOddStep);

    // diagonal /
    val += rowEval(0, i, 1, -1, player, isOddStep);
  }

  // the otherway for diagnoals
  for (int j = 1; j < BOARDSIZE; j++) {
    val += rowEval(j, 0, 1, 1, player, isOddStep);
    val += rowEval(j, BOARDSIZE - 1, 1, -1, player, isOddStep);
  }

  return val;
}

int Gomoku::rowEval(int x, int y, int dx, int dy, Piece self, bool isOddStep) {
  Piece opponent = otherPlayer(self);
  BitRowBuilder rowBuilder;
  int val = 0;
  for (int i = 0; i < BOARDSIZE; i++) {
    if (!inbound(x, y))
      break;
    Piece p = board.getPiece(x, y);
    x += dx;
    y += dy;
    if (p == opponent) {
      val += subRowEval(rowBuilder.getRow(), isOddStep);
      rowBuilder.reset();
      continue;
    }
    rowBuilder.add(p == Piece::EMPTY);
  }
  val += subRowEval(rowBuilder.getRow(), isOddStep);
  return val;
}

//
int Gomoku::singlePieceEvaluation(int x, int y, Piece player) {
  // the evaluation is decided by
  // the sum of the 6 row evals
  // of itself and the opponent

  // vertical
  int val = rowEval(0, y, 1, 0, player, true);
  // horizontal
  val += rowEval(x, 0, 0, 1, player, true);

  if (y - x >= 0) {
    val += rowEval(0, y - x, 1, 1, player, true);
  } else {
    val += rowEval(x - y, 0, 1, 1, player, true);
  }
  if (x + y < BOARDSIZE) {
    val += rowEval(0, x + y, 1, -1, player, true);
  } else {
    val += rowEval(x + y - (BOARDSIZE - 1), BOARDSIZE - 1, 1, -1, player, true);
  }
  return val;
}

// come up with a better move generation
// this is actually taking a lot of time I think
// its being computed everystep and it does multiple board evals
std::vector<Gomoku::ScoreXY> Gomoku::genBestMoves(Piece cur) {
  auto opponent = otherPlayer(cur);
  std::vector<ScoreXY> scores;
  int minX = BOARDSIZE / 2;
  int maxX = BOARDSIZE / 2 + 1;
  int minY = BOARDSIZE / 2;
  int maxY = BOARDSIZE / 2 + 1;
  for (int x = 0; x < BOARDSIZE; x++) {
    for (int y = 0; y < BOARDSIZE; y++) {
      auto p = board.getPiece(x, y);
      if (p != Piece::EMPTY) {
        minX = std::min(x, minX);
        minY = std::min(y, minY);
        maxX = std::max(x, maxX);
        maxY = std::max(y, maxY);
      }
    }
  }
  minX -= 2;
  maxX += 2;
  minY -= 2;
  maxY += 2;
  minX = std::max(0, minX);
  maxX = std::min(BOARDSIZE - 1, maxX);
  minY = std::max(0, minY);
  maxY = std::min(BOARDSIZE - 1, maxY);

  // can even do better. -> before state only need to be computed once and reused
  // but implementation is difficult

  for (int x = minX; x <= maxX; x++) {
    for (int y = minY; y <= maxY; y++) {
      auto p = board.getPiece(x, y);
      if (p == Piece::EMPTY) {
        int before = singlePieceEvaluation(x, y, cur) +
                     singlePieceEvaluation(x, y, opponent);
        board.placePiece(x, y, cur);
        // if cur can win, then just go for it
        if (singlePieceWinner(x, y)) {
          board.placePiece(x, y, Piece::EMPTY);
          return {std::make_tuple(1, x, y)};
        }
        int after1 = singlePieceEvaluation(x, y, cur);
        board.placePiece(x, y, opponent);
        int after2 = singlePieceEvaluation(x, y, opponent);
        int curScore = after1 + after2 - before;
        board.placePiece(x, y, Piece::EMPTY);
        scores.emplace_back(curScore, x, y);
      }
    }
  }
  // sorting still helps
  std::sort(scores.begin(), scores.end(),
            [](const ScoreXY &lhs, const ScoreXY &rhs) {
              return std::get<0>(lhs) > std::get<0>(rhs);
            });

  // keep top 20 scores
  // scores.resize(20);

  return scores;
}

// based on 4 steps
// odd total step pass true, false
// even total step pass false, true
Gomoku::ScoreXY Gomoku::negaMax(int depth, int alpha, int beta, Piece start,
                                Piece next) {
  auto opponent = otherPlayer(start);

  // early termination is weird...
  // 4 B
  // 3 W
  // 2 B
  // 1 W <- if winner at this step then scoreOf(W) - scoreOf(B)
  // 0 B <- this = scoreOf(B) - scoreOf(W) by default

  if (checkWinner()) {
    auto realOpponent = otherPlayer(next);
    int score = evalBoard(next, true) - evalBoard(realOpponent, true);
    return std::make_tuple(score, -1, -1);
  }
  if (depth == 0) {
    return std::make_tuple(evalBoard(start, false) - evalBoard(opponent, true),
                           -1, -1);
  }

  int bestX = -1;
  int bestY = -1;
  int bestVal = -99999999;

  for (const auto &scoreXY : genBestMoves(next)) {
    int x = std::get<1>(scoreXY);
    int y = std::get<2>(scoreXY);
    board.placePiece(x, y, next);
    auto nextScoreXY =
        negaMax(depth - 1, -1 * beta, -1 * alpha, start, otherPlayer(next));
    int v = -1 * std::get<0>(nextScoreXY);

    if (v > bestVal) {
      bestX = x;
      bestY = y;
      bestVal = v;
    }
    board.placePiece(x, y, Piece::EMPTY);
    alpha = std::max(alpha, v);
    if (beta <= alpha)
      break;
  }

  return std::make_tuple(bestVal, bestX, bestY);
}

int Gomoku::checkWinner() {
  for (int x = 0; x <= BOARDSIZE; x++) {
    for (int y = 0; y <= BOARDSIZE; y++) {
      auto winner = singlePieceWinner(x, y);
      if (winner) {
        return winner;
      }
    }
  }
  return 0;
}

int Gomoku::singlePieceWinner(int x, int y) {
  auto p = board.getPiece(x, y);
  if (p == Piece::EMPTY) {
    return 0;
  }
  int dirx[] = {0, 1, 1, 1};
  int diry[] = {-1, -1, 0, 1};
  int dirs[] = {1, -1};
  // how do I zip dirx diry
  // this is wrong!
  for (int d = 0; d < 4; d++) {
    int count = 0;
    for (int s = 0; s < 2; s++) {
      int nx = x;
      int ny = y;
      for (int i = 0; i <= 4; i++) {
        nx = nx + dirs[s] * dirx[d];
        ny = ny + dirs[s] * diry[d];
        if (!inbound(nx, ny)) {
          break;
        }
        if (board.getPiece(nx, ny) != p) {
          break;
        }
        count++;
      }
      if (count >= 4)
        return (int)p;
    }
  }
  return 0;
}

std::ostream &operator<<(std::ostream &stream, const Gomoku &gomoku) {
  stream << "Player " << gomoku.turn << "'s turn" << std::endl;
  stream << gomoku.board << std::endl;
  return stream;
}
