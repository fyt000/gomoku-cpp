#include "Gomoku.h"
#include "BitRowBuilder.h"
#include <algorithm>
#include <chrono>
#include <future>

// std::mutex Gomoku::ttLock;
// std::unordered_map<Board, TTEntry, BoardHasher> Gomoku::transposition;

Gomoku::Gomoku() {}

Gomoku::Gomoku(const std::vector<int> &patternLookup1,
               const std::vector<int> &patternLookup2)
    : patternLookup1(patternLookup1), patternLookup2(patternLookup2) {
  maxScore = (*std::max_element(patternLookup1.begin(), patternLookup1.end()));
  maxScore = std::max(maxScore, (*std::max_element(patternLookup2.begin(),
                                                   patternLookup2.end())));
  wonScore = 2 * maxScore;
}

bool Gomoku::placePiece(int x, int y) {
  if (curBoard.getPiece(x, y) != Piece::EMPTY)
    return false;
  curBoard.placePiece(x, y, turn);
  turn = otherPlayer(turn);
  return true;
}

std::pair<int, int> Gomoku::placePiece(int maxDepth, bool threaded) {
  // std::cout<<sizeof(curBoard)<<std::endl;
  int x;
  int y;
  int score;
  nodesVisited.store(0);
  {
    std::lock_guard<std::mutex> lock(ttLock);
    transposition.clear();
  }
  

  auto t1 = std::chrono::high_resolution_clock::now();

  ScoreXY p;
  if (threaded) {
    p = multithreadSearch(curBoard, maxDepth, -99999999, 99999999, turn);
  } else {
    for (int depth = maxDepth; depth <= maxDepth; depth++) {
      Board boardCopy(curBoard);
      p = negaScout(boardCopy, depth, -99999999, 99999999, turn, turn);
      // firstGuess = p;
      std::cerr << "next first guess " << std::get<1>(p) << " "
                << std::get<2>(p) << std::endl;
    }
  }

  auto t2 = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
  std::cout << duration << '\t' << nodesVisited.load() << std::endl;

  // std::cout << "transposition size " << transposition.size() << std::endl;

  x = std::get<1>(p);
  y = std::get<2>(p);
  // lost already
  if (x == -1 && y == -1) {
    std::cout << "go add best move in TTEntry" << std::endl;
    auto anyP = genBestMoves(curBoard, turn)[0];
    x = std::get<1>(anyP);
    y = std::get<2>(anyP);
  }
  placePiece(x, y);
  return {x, y};
}

int Gomoku::evalBoard(Board &board, Piece player, bool isOddStep) {
  int val = 0;

  // for better locality
  // store these directions each in an array
  // but also find a mapping from placing a piece x,y
  // to update the board

  for (int i = 0; i < BOARDSIZE; i++) {
    // vertical
    val += rowEval(board, 0, i, 1, 0, player, isOddStep);
    // horizontal
    val += rowEval(board, i, 0, 0, 1, player, isOddStep);

    // diagonal '\'
    val += rowEval(board, 0, i, 1, 1, player, isOddStep);

    // diagonal /
    val += rowEval(board, 0, i, 1, -1, player, isOddStep);
  }

  // the otherway for diagnoals
  for (int j = 1; j < BOARDSIZE; j++) {
    val += rowEval(board, j, 0, 1, 1, player, isOddStep);
    val += rowEval(board, j, BOARDSIZE - 1, 1, -1, player, isOddStep);
  }

  return val;
}

int Gomoku::rowEval(Board &board, int x, int y, int dx, int dy, Piece self,
                    bool isOddStep) {
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
int Gomoku::singlePieceEvaluation(Board &board, int x, int y, Piece player) {
  // the evaluation is decided by
  // the sum of the 6 row evals

  // vertical
  int val = rowEval(board, 0, y, 1, 0, player, true);
  // horizontal
  val += rowEval(board, x, 0, 0, 1, player, true);

  if (y - x >= 0) {
    val += rowEval(board, 0, y - x, 1, 1, player, true);
  } else {
    val += rowEval(board, x - y, 0, 1, 1, player, true);
  }
  if (x + y < BOARDSIZE) {
    val += rowEval(board, 0, x + y, 1, -1, player, true);
  } else {
    val += rowEval(board, x + y - (BOARDSIZE - 1), BOARDSIZE - 1, 1, -1, player,
                   true);
  }
  return val;
}

// come up with a better move generation
// this is actually taking a lot of time I think
// its being computed everystep and it does multiple board evals
std::vector<Gomoku::ScoreXY> Gomoku::genBestMoves(Board &board, Piece cur) {
  auto opponent = otherPlayer(cur);
  std::vector<ScoreXY> scores;

#define SETNEW(val, newVal, maxFunc)                                           \
  {                                                                            \
    if (val == -1) {                                                           \
      val = newVal;                                                            \
    } else {                                                                   \
      val = maxFunc(val, newVal);                                              \
    }                                                                          \
  }

  int minX = -1;
  int maxX = -1;
  int minY = -1;
  int maxY = -1;
  for (int x = 0; x < BOARDSIZE; x++) {
    for (int y = 0; y < BOARDSIZE; y++) {
      auto p = board.getPiece(x, y);
      if (p != Piece::EMPTY) {
        SETNEW(minX, x, std::min);
        SETNEW(minY, y, std::min);
        SETNEW(maxX, x, std::max);
        SETNEW(maxY, y, std::max);
      }
    }
  }
  if (minX == -1 && maxX == -1 && minY == -1 && maxY == -1) {
    minX = 7;
    maxX = 7;
    minY = 7;
    maxY = 7;
  }

  minX -= 2;
  maxX += 2;
  minY -= 2;
  maxY += 2;
  minX = std::max(0, minX);
  maxX = std::min(BOARDSIZE - 1, maxX);
  minY = std::max(0, minY);
  maxY = std::min(BOARDSIZE - 1, maxY);

  // can even do better. -> before state only need to be computed once and
  // reused
  // but implementation is difficult

  for (int x = minX; x <= maxX; x++) {
    for (int y = minY; y <= maxY; y++) {
      auto p = board.getPiece(x, y);
      if (p == Piece::EMPTY) {
        int before = singlePieceEvaluation(board, x, y, cur) +
                     singlePieceEvaluation(board, x, y, opponent);
        board.placePiece(x, y, cur, false);
        // if cur can win, then just go for it
        if (singlePieceWinner(board, x, y)) {
          board.undoPiece(x, y, false);
          return {std::make_tuple(1, x, y)};
        }
        int after1 = singlePieceEvaluation(board, x, y, cur);
        board.placePiece(x, y, opponent, false);
        int after2 = singlePieceEvaluation(board, x, y, opponent);
        int curScore = after1 + after2 - before;
        board.undoPiece(x, y, false);
        scores.emplace_back(curScore, x, y);
      }
    }
  }
  // sorting still helps
  std::sort(scores.begin(), scores.end(),
            [](const ScoreXY &lhs, const ScoreXY &rhs) {
              return std::get<0>(lhs) > std::get<0>(rhs);
            });
  // any resize makes it real weak
  // scores.resize(25);

  return scores;
}

Gomoku::ScoreXY Gomoku::updateTTAndRet(Board &board, int score, int alpha,
                                       int beta, int depth, int bestX,
                                       int bestY) {
  std::lock_guard<std::mutex> lock(ttLock);
  auto ttEntryPair = transposition.find(board);
  TType type;
  if (score <= alpha) {
    type = TType::LOWER;
  } else if (score >= beta) {
    type = TType::UPPER;
  } else {
    type = TType::EXACT;
  }
  if (ttEntryPair == transposition.end()) {
    transposition[board] = TTEntry(score, type, depth);
  } else {
    auto &ttEntry = ttEntryPair->second;
    if (ttEntry.depth < depth) {
      ttEntry.type = type;
      ttEntry.value = score;
      ttEntry.depth = depth;
    }
  }
  return std::make_tuple(score, bestX, bestY);
}

Gomoku::ScoreXY Gomoku::multithreadSearch(Board &board, int depth, int alpha,
                                          int beta, Piece start) {
  int threadNum = 8;
  std::vector<std::thread> searcher;
  std::vector<std::future<ScoreXY>> results;
  auto moves = genBestMoves(board, start);

  int bestX = -1;
  int bestY = -1;
  int bestVal = -99999999;

  int x = std::get<1>(moves[0]);
  int y = std::get<2>(moves[0]);
  board.placePiece(x, y, start);
  auto scoreXY =
      negaScout(board, depth - 1, -beta, -alpha, start, otherPlayer(start));
  board.undoPiece(x, y);
  int v = -1 * std::get<0>(scoreXY);
  if (v > bestVal) {
    bestX = x;
    bestY = y;
    bestVal = v;
  }

  board.undoPiece(x, y);
  // update the new alpha
  alpha = std::max(alpha, v);
  int startAt = 1;

  std::cerr << "start distributing work on "<<moves.size() << std::endl;
  int threadIdx = 0; 
  // distribute the work
  std::vector<std::vector<ScoreXY>> movesSplits(threadNum);
  for (int i = startAt; i < moves.size(); i++) {
    movesSplits[threadIdx++].push_back(moves[i]);
    threadIdx = threadIdx % threadNum;
  }

  for (int i = 0; i < threadNum; i++) {
    std::promise<ScoreXY> r;
    results.emplace_back(r.get_future());
    // make many copies
    std::vector<ScoreXY> threadMoves(movesSplits[i]);
    searcher.emplace_back(
        [this, threadMoves, depth, alpha, beta, start](std::promise<ScoreXY> &&r) {
          Board boardCopy(curBoard);
          auto p = negaScoutWorker(boardCopy, depth, alpha, beta, start, start,
                                   threadMoves);
          r.set_value(p);
        },
        std::move(r));
  }

  for (auto &t : searcher) {
    t.join();
  }
  for (auto &r : results) {
    auto theMove = r.get();
    if (std::get<0>(theMove) > bestVal) {
      bestVal = std::get<0>(theMove);
      bestX = std::get<1>(theMove);
      bestY = std::get<2>(theMove);
    }
  }
  // std::cout<<"placing on "<<bestX<<" "<<bestY<<std::endl;
  return std::make_tuple(bestVal, bestX, bestY);
}

Gomoku::ScoreXY Gomoku::negaScoutWorker(Board &board, int depth, int alpha,
                                        int beta, Piece start, Piece next,
                                        const std::vector<ScoreXY> &moves) {
  // no transposition look up
  int bestX = -1;
  int bestY = -1;
  auto opponent = otherPlayer(start);
  int bestVal = alpha;

  ScoreXY nextScoreXY;
  for (const auto &scoreXY : moves) {

    int x = std::get<1>(scoreXY);
    int y = std::get<2>(scoreXY);
    // std::cerr << "doing work on " << x << " " << y << std::endl;
    board.placePiece(x, y, next);
    ScoreXY nextScoreXY;

    // search with null window as alpha is already updated
    nextScoreXY = negaScout(board, depth - 1, -alpha - 1, -alpha, start,
                            otherPlayer(next));
    int v = -1 * std::get<0>(nextScoreXY);
    if (alpha < v && v < beta) {
      nextScoreXY =
          negaScout(board, depth - 1, -beta, -v, start, otherPlayer(next));
    }


    // nextScoreXY = negaScout(board, depth - 1, -beta, -alpha, start, otherPlayer(next));
    // int v = -1 * std::get<0>(nextScoreXY);

    if (v > bestVal) {
      bestX = x;
      bestY = y;
      bestVal = v;
    }
    board.undoPiece(x, y);
    alpha = std::max(alpha, v);
    if (beta <= alpha)
      break;
  }
  return updateTTAndRet(board, bestVal, alpha, beta, depth, bestX, bestY);
}

Gomoku::ScoreXY Gomoku::negaScout(Board &board, int depth, int alpha, int beta,
                                  Piece start, Piece next) {
  int bestX = -1;
  int bestY = -1;

  // A tt hit always returns (-1,-1)
  // this only works because tt gets cleared each placePiece
  // and thus, everytime we call negaScout from root
  // the root will be a miss
  // therefore, we don't need the best step
  // as the loop will find one for us
  {
    std::lock_guard<std::mutex> lock(ttLock);
    auto ttEntryPair = transposition.find(board);
    // why higher depth?
    if (ttEntryPair != transposition.end()) {
      // std::cerr<<"found same board\n";
      auto &ttEntry = ttEntryPair->second;
      if (ttEntry.depth >= depth) {
        // std::cerr << ttEntryPair->first;
        if (ttEntry.type == TType::EXACT) {
          return std::make_tuple(ttEntry.value, -1, -1);
        } else if (ttEntry.type == TType::LOWER) {
          alpha = std::max(alpha, ttEntry.value);
        } else if (ttEntry.type == TType::UPPER) {
          beta = std::min(beta, ttEntry.value);
        }
        if (alpha >= beta)
          return std::make_tuple(ttEntry.value, -1, -1);
      }
    }
  }

  auto opponent = otherPlayer(start);

  // early termination is weird...
  // 4 B
  // 3 W
  // 2 B
  // 1 W <- if winner at this step then scoreOf(W) - scoreOf(B)
  // 0 B <- this = scoreOf(B,f) - scoreOf(W,t) by default

  // if odd number of steps...
  // requires:
  // 1 B
  // 0 W <- scoreOf(B,t) - scoreOf(W,f)

  // 3 B
  // 2 W <- winner at this step, then black won, so we need -ve score
  // 1 B <- winner at this step, white won, again negative
  // 0 W <- winner at this step, black won, negative

  // if I add iterative deepening
  // how does this work?
  int score;
  int winner = checkWinner(board);
  if (winner) {
    nodesVisited++;
    // favour early wins
    // so don't do stupid stuff
    // if someone won, it will not be next!
    // so the coe should always be negative!
    // int coe = winner == (int)next ? 1 : -1;
    int coe = -1;
    score = maxScore * (depth + 1) * coe;
    return updateTTAndRet(board, score, alpha, beta, depth, bestX, bestY);
  }
  if (depth == 0) {
    nodesVisited++;
    // always evaluate in terms of the start player is wrong!
    // it works for even steps but not odd
    // because eval is not symetric I guess transposition doesn't work that well
    bool totalOdd = next != start;
    if (!totalOdd) {
      // even steps
      score = evalBoard(board, start, totalOdd) -
              evalBoard(board, opponent, !totalOdd);
    } else {
      // odd step
      score = evalBoard(board, opponent, !totalOdd) -
              evalBoard(board, start, totalOdd);
    }

    return updateTTAndRet(board, score, alpha, beta, depth, bestX, bestY);
  }

  int bestVal = -99999999;

  bool firstNode = true;
  auto moves = genBestMoves(board, next);

  for (const auto &scoreXY : moves) {
    int x = std::get<1>(scoreXY);
    int y = std::get<2>(scoreXY);
    board.placePiece(x, y, next);
    ScoreXY nextScoreXY;
    if (firstNode) {
      nextScoreXY =
          negaScout(board, depth - 1, -beta, -alpha, start, otherPlayer(next));
      firstNode = false;
    } else {
      nextScoreXY = negaScout(board, depth - 1, -alpha - 1, -alpha, start,
                              otherPlayer(next));
      int v = -1 * std::get<0>(nextScoreXY);
      if (alpha < v && v < beta) {
        nextScoreXY =
            negaScout(board, depth - 1, -beta, -v, start, otherPlayer(next));
      }
    }
    int v = -1 * std::get<0>(nextScoreXY);
    // if (depth == 5) {
    //   std::cerr << v << std::endl;
    //   std::cerr << board;
    // }

    if (v > bestVal) {
      bestX = x;
      bestY = y;
      bestVal = v;
    }
    board.undoPiece(x, y);
    alpha = std::max(alpha, v);
    if (beta <= alpha)
      break;
  }

  return updateTTAndRet(board, bestVal, alpha, beta, depth, bestX, bestY);
}

int Gomoku::checkWinner() { checkWinner(curBoard); }

int Gomoku::checkWinner(Board &board) {
  for (int x = 0; x <= BOARDSIZE; x++) {
    for (int y = 0; y <= BOARDSIZE; y++) {
      auto winner = singlePieceWinner(board, x, y);
      if (winner) {
        return winner;
      }
    }
  }
  return 0;
}

int Gomoku::singlePieceWinner(Board &board, int x, int y) {
  auto p = board.getPiece(x, y);
  if (p == Piece::EMPTY) {
    return 0;
  }
  int dirx[] = {0, 1, 1, 1};
  int diry[] = {-1, -1, 0, 1};
  int dirs[] = {1, -1};
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
        return (unsigned char)p;
    }
  }
  return 0;
}

std::ostream &operator<<(std::ostream &stream, const Gomoku &gomoku) {
  stream << "Player " << gomoku.turn << "'s turn" << std::endl;
  stream << gomoku.curBoard << std::endl;
  return stream;
}
