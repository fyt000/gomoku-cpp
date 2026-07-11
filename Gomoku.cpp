#include "Gomoku.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <future>
#include <limits>
#include <ostream>
#include <utility>
#include <vector>

namespace gomoku {

namespace {

using Score = std::int64_t;

constexpr Score kMateScore = 1'000'000'000'000LL;
constexpr Score kInfinity = std::numeric_limits<Score>::max() / 4;
constexpr unsigned int kMaximumRootThreads = 8;
constexpr std::size_t kLineCount = 72;
constexpr std::size_t kCellCount = kBoardSize * kBoardSize;

struct LineDefinition {
  std::uint8_t startX = 0;
  std::uint8_t startY = 0;
  std::int8_t deltaX = 0;
  std::int8_t deltaY = 0;
  std::uint8_t length = 0;
};

struct CellLineReference {
  std::uint8_t line = 0;
  std::uint16_t bit = 0;
};

struct CellGeometry {
  std::array<CellLineReference, 4> references{};
  std::uint8_t count = 0;
};

struct LineGeometry {
  std::array<LineDefinition, kLineCount> lines{};
  std::array<CellGeometry, kCellCount> cells{};
};

const LineGeometry& lineGeometry()
{
  static const LineGeometry geometry = [] {
    LineGeometry result{};
    std::size_t lineCount = 0;

    const auto addLine = [&](int startX, int startY, int deltaX, int deltaY) {
      int length = 0;
      int x = startX;
      int y = startY;
      while (x >= 0 && x < kBoardSize && y >= 0 && y < kBoardSize) {
        ++length;
        x += deltaX;
        y += deltaY;
      }
      if (length < 5) {
        return;
      }

      assert(lineCount < result.lines.size());
      result.lines[lineCount] = {.startX = static_cast<std::uint8_t>(startX),
                                 .startY = static_cast<std::uint8_t>(startY),
                                 .deltaX = static_cast<std::int8_t>(deltaX),
                                 .deltaY = static_cast<std::int8_t>(deltaY),
                                 .length = static_cast<std::uint8_t>(length)};

      x = startX;
      y = startY;
      for (int offset = 0; offset < length; ++offset) {
        CellGeometry& cell = result.cells[static_cast<std::size_t>(x * kBoardSize + y)];
        assert(cell.count < cell.references.size());
        cell.references[cell.count++] = {
            .line = static_cast<std::uint8_t>(lineCount),
            .bit = static_cast<std::uint16_t>(1u << (length - 1 - offset))};
        x += deltaX;
        y += deltaY;
      }
      ++lineCount;
    };

    for (int row = 0; row < kBoardSize; ++row) {
      addLine(row, 0, 0, 1);
    }
    for (int column = 0; column < kBoardSize; ++column) {
      addLine(0, column, 1, 0);
    }
    for (int column = 0; column <= kBoardSize - 5; ++column) {
      addLine(0, column, 1, 1);
    }
    for (int row = 1; row <= kBoardSize - 5; ++row) {
      addLine(row, 0, 1, 1);
    }
    for (int column = 4; column < kBoardSize; ++column) {
      addLine(0, column, 1, -1);
    }
    for (int row = 1; row <= kBoardSize - 5; ++row) {
      addLine(row, kBoardSize - 1, 1, -1);
    }

    assert(lineCount == result.lines.size());
    return result;
  }();
  return geometry;
}

std::uint64_t splitMix64(std::uint64_t& state)
{
  std::uint64_t value = (state += 0x9E3779B97F4A7C15ULL);
  value = (value ^ (value >> 30)) * 0xBF58476D1CE4E5B9ULL;
  value = (value ^ (value >> 27)) * 0x94D049BB133111EBULL;
  return value ^ (value >> 31);
}

struct ZobristKeys {
  std::array<std::array<std::uint64_t, 2>, kCellCount> pieces{};
  std::array<std::uint64_t, 2> sideToMove{};
};

const ZobristKeys& zobristKeys()
{
  static const ZobristKeys keys = [] {
    ZobristKeys result{};
    std::uint64_t state = 0xD1B54A32D192ED03ULL;
    for (auto& cell : result.pieces) {
      cell[0] = splitMix64(state);
      cell[1] = splitMix64(state);
    }
    result.sideToMove[0] = splitMix64(state);
    result.sideToMove[1] = splitMix64(state);
    return result;
  }();
  return keys;
}

enum class BoundType : std::uint8_t { EXACT, LOWER, UPPER };

struct TranspositionEntry {
  std::uint64_t key = 0;
  Score value = 0;
  std::uint32_t generation = 0;
  std::int16_t depth = -1;
  std::int8_t bestX = -1;
  std::int8_t bestY = -1;
  BoundType bound = BoundType::EXACT;
};

class TranspositionTable {
 public:
  void beginSearch()
  {
    ++generation;
    if (generation == 0) {
      std::fill(entries.begin(), entries.end(), TranspositionEntry{});
      generation = 1;
    }
  }

  [[nodiscard]] const TranspositionEntry* find(std::uint64_t key) const
  {
    const TranspositionEntry& entry = entries[static_cast<std::size_t>(key) & (entries.size() - 1)];
    if (entry.generation == generation && entry.key == key) {
      return &entry;
    }
    return nullptr;
  }

  void store(std::uint64_t key, int depth, Score value, BoundType bound, int bestX, int bestY)
  {
    TranspositionEntry& entry = entries[static_cast<std::size_t>(key) & (entries.size() - 1)];
    if (entry.generation == generation && entry.depth > depth) {
      return;
    }
    entry = {.key = key,
             .value = value,
             .generation = generation,
             .depth = static_cast<std::int16_t>(depth),
             .bestX = static_cast<std::int8_t>(bestX),
             .bestY = static_cast<std::int8_t>(bestY),
             .bound = bound};
  }

 private:
  static constexpr std::size_t kEntryCount = 1u << 17;
  std::vector<TranspositionEntry> entries = std::vector<TranspositionEntry>(kEntryCount);
  std::uint32_t generation = 0;
};

thread_local TranspositionTable transpositionTable;

bool containsFive(std::uint16_t mask)
{
  return (mask & static_cast<std::uint16_t>(mask >> 1) & static_cast<std::uint16_t>(mask >> 2) &
          static_cast<std::uint16_t>(mask >> 3) & static_cast<std::uint16_t>(mask >> 4)) != 0;
}

}  // namespace

Gomoku::Gomoku() : Gomoku(zeroEvaluationTables()) {}

Gomoku::Gomoku(EvaluationTablesPtr evaluationTables)
    : evaluationTables(evaluationTables ? std::move(evaluationTables) : zeroEvaluationTables())
{
  rebuildDerivedState();
}

EvaluationTablesPtr Gomoku::zeroEvaluationTables()
{
  static const EvaluationTablesPtr tables = std::make_shared<const EvaluationTables>();
  return tables;
}

Piece Gomoku::otherPlayer(Piece piece)
{
  return piece == Piece::WHITE ? Piece::BLACK : Piece::WHITE;
}

int Gomoku::playerIndex(Piece piece)
{
  return piece == Piece::WHITE ? 1 : 0;
}

std::size_t Gomoku::scoreIndex(Piece piece, bool isOddStep)
{
  return static_cast<std::size_t>(playerIndex(piece) * 2 + (isOddStep ? 0 : 1));
}

bool Gomoku::placePiece(int x, int y)
{
  if (x < 0 || x >= kBoardSize || y < 0 || y >= kBoardSize ||
      board.getPiece(x, y) != Piece::EMPTY) {
    return false;
  }
  setPieceInternal(x, y, turn);
  turn = otherPlayer(turn);
  return true;
}

std::pair<int, int> Gomoku::placePiece()
{
  return placePiece(SearchOptions{});
}

std::pair<int, int> Gomoku::placePiece(int searchDepth)
{
  return placePiece(SearchOptions{.depth = searchDepth});
}

std::pair<int, int> Gomoku::placePiece(SearchOptions options)
{
  if (checkWinner() != Piece::EMPTY || isBoardFull()) {
    return {-1, -1};
  }

  if (board.isEmpty()) {
    constexpr int center = kBoardSize / 2;
    (void)placePiece(center, center);
    return {center, center};
  }

  transpositionTable.beginSearch();
  const int depth = std::max(1, options.depth);
  const int frontierRadius = std::clamp(options.frontierRadius, 1, 4);
  const auto move = options.threadCount > 1
                        ? parallelRootSearch(depth, turn, frontierRadius, options.threadCount)
                        : negaMax(depth, -kInfinity, kInfinity, turn, -1, -1, frontierRadius);
  int x = std::get<1>(move);
  int y = std::get<2>(move);

  if (x == -1 && y == -1) {
    const MoveList moves = genBestMoves(turn, frontierRadius);
    if (moves.size == 0) {
      return {-1, -1};
    }
    x = moves.moves[0].x;
    y = moves.moves[0].y;
  }

  (void)placePiece(x, y);
  return {x, y};
}

void Gomoku::reset()
{
  board = Board{};
  turn = Piece::BLACK;
  rebuildDerivedState();
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
  return board.isFull();
}

Gomoku::Score Gomoku::subRowEval(int subRow, bool isOddStep) const
{
  if (subRow <= (1 << 5)) {
    return 0;
  }
  const auto& lookup = isOddStep ? evaluationTables->oddStep : evaluationTables->evenStep;
  return lookup[static_cast<std::size_t>(subRow)];
}

void Gomoku::recomputeLine(std::size_t lineIndex)
{
  const int length = lineGeometry().lines[lineIndex].length;
  LineState& line = lines[lineIndex];
  line.scores = {};
  int blackRow = 1;
  int whiteRow = 1;

  const auto finishSubRow = [this, &line](Piece player, int subRow) {
    line.scores[scoreIndex(player, true)] += subRowEval(subRow, true);
    line.scores[scoreIndex(player, false)] += subRowEval(subRow, false);
  };

  for (int offset = 0; offset < length; ++offset) {
    const std::uint16_t bit = static_cast<std::uint16_t>(1u << (length - 1 - offset));
    if ((line.white & bit) != 0) {
      finishSubRow(Piece::BLACK, blackRow);
      blackRow = 1;
    } else {
      blackRow = (blackRow << 1) | ((line.black & bit) != 0 ? 1 : 0);
    }

    if ((line.black & bit) != 0) {
      finishSubRow(Piece::WHITE, whiteRow);
      whiteRow = 1;
    } else {
      whiteRow = (whiteRow << 1) | ((line.white & bit) != 0 ? 1 : 0);
    }
  }

  finishSubRow(Piece::BLACK, blackRow);
  finishSubRow(Piece::WHITE, whiteRow);
}

void Gomoku::rebuildDerivedState()
{
  lines = {};
  totalScores = {};
  positionHash = 0;
  const LineGeometry& geometry = lineGeometry();

  for (std::size_t lineIndex = 0; lineIndex < geometry.lines.size(); ++lineIndex) {
    const LineDefinition& definition = geometry.lines[lineIndex];
    int x = definition.startX;
    int y = definition.startY;
    for (int offset = 0; offset < definition.length; ++offset) {
      const std::uint16_t bit = static_cast<std::uint16_t>(1u << (definition.length - 1 - offset));
      const Piece piece = board.getPiece(x, y);
      if (piece == Piece::BLACK) {
        lines[lineIndex].black |= bit;
      } else if (piece == Piece::WHITE) {
        lines[lineIndex].white |= bit;
      }
      x += definition.deltaX;
      y += definition.deltaY;
    }
    recomputeLine(lineIndex);
    for (std::size_t score = 0; score < totalScores.size(); ++score) {
      totalScores[score] += lines[lineIndex].scores[score];
    }
  }

  const ZobristKeys& keys = zobristKeys();
  for (int x = 0; x < kBoardSize; ++x) {
    for (int y = 0; y < kBoardSize; ++y) {
      const Piece piece = board.getPiece(x, y);
      if (piece != Piece::EMPTY) {
        positionHash ^=
            keys.pieces[static_cast<std::size_t>(x * kBoardSize + y)][playerIndex(piece)];
      }
    }
  }
}

void Gomoku::setPieceInternal(int x, int y, Piece piece)
{
  const Piece previous = board.getPiece(x, y);
  if (previous == piece) {
    return;
  }

  const CellGeometry& cell = lineGeometry().cells[static_cast<std::size_t>(x * kBoardSize + y)];
  for (int referenceIndex = 0; referenceIndex < cell.count; ++referenceIndex) {
    const CellLineReference& reference = cell.references[static_cast<std::size_t>(referenceIndex)];
    LineState& line = lines[reference.line];
    for (std::size_t score = 0; score < totalScores.size(); ++score) {
      totalScores[score] -= line.scores[score];
    }

    if (previous == Piece::BLACK) {
      line.black &= static_cast<std::uint16_t>(~reference.bit);
    } else if (previous == Piece::WHITE) {
      line.white &= static_cast<std::uint16_t>(~reference.bit);
    }
    if (piece == Piece::BLACK) {
      line.black |= reference.bit;
    } else if (piece == Piece::WHITE) {
      line.white |= reference.bit;
    }

    recomputeLine(reference.line);
    for (std::size_t score = 0; score < totalScores.size(); ++score) {
      totalScores[score] += line.scores[score];
    }
  }

  const ZobristKeys& keys = zobristKeys();
  const std::size_t cellIndex = static_cast<std::size_t>(x * kBoardSize + y);
  if (previous != Piece::EMPTY) {
    positionHash ^= keys.pieces[cellIndex][playerIndex(previous)];
  }
  if (piece != Piece::EMPTY) {
    positionHash ^= keys.pieces[cellIndex][playerIndex(piece)];
  }
  board.placePiece(x, y, piece);
}

Gomoku::Score Gomoku::totalScore(Piece player, bool isOddStep) const
{
  return totalScores[scoreIndex(player, isOddStep)];
}

Gomoku::MoveList Gomoku::genBestMoves(Piece current, int frontierRadius)
{
  const Piece opponent = otherPlayer(current);
  const Score beforeScore = totalScore(current, true) + totalScore(opponent, true);
  const Board::Bitboard candidates = board.frontier(frontierRadius);
  MoveList result;

  for (std::size_t wordIndex = 0; wordIndex < candidates.size(); ++wordIndex) {
    std::uint64_t word = candidates[wordIndex];
    while (word != 0) {
      const int bitIndex = static_cast<int>(std::countr_zero(word));
      word &= word - 1;
      const int padded = static_cast<int>(wordIndex * 64 + bitIndex);
      const int x = padded / kBoardStride;
      const int y = padded % kBoardStride;

      setPieceInternal(x, y, current);
      if (singlePieceWinner(x, y)) {
        setPieceInternal(x, y, Piece::EMPTY);
        result.moves[0] = {.score = kMateScore,
                           .x = static_cast<std::uint8_t>(x),
                           .y = static_cast<std::uint8_t>(y)};
        result.size = 1;
        return result;
      }
      const Score currentScore = totalScore(current, true);
      setPieceInternal(x, y, Piece::EMPTY);

      setPieceInternal(x, y, opponent);
      const Score score = currentScore + totalScore(opponent, true) - beforeScore;
      setPieceInternal(x, y, Piece::EMPTY);

      result.moves[result.size++] = {
          .score = score, .x = static_cast<std::uint8_t>(x), .y = static_cast<std::uint8_t>(y)};
    }
  }

  std::sort(result.moves.begin(), result.moves.begin() + static_cast<std::ptrdiff_t>(result.size),
            [](const ScoredMove& lhs, const ScoredMove& rhs) { return lhs.score > rhs.score; });
  return result;
}

Gomoku::ScoreXY Gomoku::parallelRootSearch(int depth, Piece next, int frontierRadius,
                                           unsigned int threadCount)
{
  MoveList moves = genBestMoves(next, frontierRadius);
  if (moves.size == 0) {
    return std::make_tuple(0, -1, -1);
  }

  std::array<Score, kMaximumMoveCount> values{};
  values.fill(-kInfinity);

  const int firstX = moves.moves[0].x;
  const int firstY = moves.moves[0].y;
  setPieceInternal(firstX, firstY, next);
  const auto firstScore =
      negaMax(depth - 1, -kInfinity, kInfinity, otherPlayer(next), firstX, firstY, frontierRadius);
  values[0] = -std::get<0>(firstScore);
  setPieceInternal(firstX, firstY, Piece::EMPTY);

  std::atomic<std::size_t> nextMove{1};
  std::atomic<Score> sharedAlpha{values[0]};
  const unsigned int workers =
      std::min({threadCount, static_cast<unsigned int>(moves.size), kMaximumRootThreads});

  const auto searchMoves = [this, &moves, &values, &nextMove, &sharedAlpha, depth, next,
                            frontierRadius](bool beginThreadSearch) {
    Gomoku worker = *this;
    if (beginThreadSearch) {
      transpositionTable.beginSearch();
    }

    while (true) {
      const std::size_t moveIndex = nextMove.fetch_add(1, std::memory_order_relaxed);
      if (moveIndex >= moves.size) {
        return;
      }

      const int x = moves.moves[moveIndex].x;
      const int y = moves.moves[moveIndex].y;
      const Score alpha = sharedAlpha.load(std::memory_order_relaxed);
      worker.setPieceInternal(x, y, next);
      const auto child =
          worker.negaMax(depth - 1, -kInfinity, -alpha, otherPlayer(next), x, y, frontierRadius);
      const Score value = -std::get<0>(child);
      worker.setPieceInternal(x, y, Piece::EMPTY);
      values[moveIndex] = value;

      Score current = sharedAlpha.load(std::memory_order_relaxed);
      while (value > current &&
             !sharedAlpha.compare_exchange_weak(current, value, std::memory_order_relaxed)) {
      }
    }
  };

  std::vector<std::future<void>> futures;
  futures.reserve(workers > 0 ? workers - 1 : 0);
  for (unsigned int worker = 1; worker < workers; ++worker) {
    futures.emplace_back(std::async(std::launch::async, searchMoves, true));
  }
  searchMoves(false);
  for (auto& future : futures) {
    future.get();
  }

  int bestX = firstX;
  int bestY = firstY;
  Score bestValue = values[0];
  for (std::size_t moveIndex = 1; moveIndex < moves.size; ++moveIndex) {
    if (values[moveIndex] > bestValue) {
      bestValue = values[moveIndex];
      bestX = moves.moves[moveIndex].x;
      bestY = moves.moves[moveIndex].y;
    }
  }

  const std::uint64_t key = positionHash ^ zobristKeys().sideToMove[playerIndex(next)];
  transpositionTable.store(key, depth, bestValue, BoundType::EXACT, bestX, bestY);
  return std::make_tuple(bestValue, bestX, bestY);
}

Gomoku::ScoreXY Gomoku::negaMax(int depth, Score alpha, Score beta, Piece next, int lastX,
                                int lastY, int frontierRadius)
{
  if (lastX >= 0 && singlePieceWinner(lastX, lastY)) {
    return std::make_tuple(-kMateScore - depth, -1, -1);
  }

  const std::uint64_t key = positionHash ^ zobristKeys().sideToMove[playerIndex(next)];
  const Score originalAlpha = alpha;
  const Score originalBeta = beta;
  int tableX = -1;
  int tableY = -1;
  if (const TranspositionEntry* entry = transpositionTable.find(key)) {
    tableX = entry->bestX;
    tableY = entry->bestY;
    if (entry->depth >= depth) {
      if (entry->bound == BoundType::EXACT) {
        return std::make_tuple(entry->value, tableX, tableY);
      }
      if (entry->bound == BoundType::LOWER) {
        alpha = std::max(alpha, entry->value);
      } else {
        beta = std::min(beta, entry->value);
      }
      if (alpha >= beta) {
        return std::make_tuple(entry->value, tableX, tableY);
      }
    }
  }

  if (depth == 0) {
    const Piece opponent = otherPlayer(next);
    const Score value = totalScore(next, false) - totalScore(opponent, true);
    transpositionTable.store(key, depth, value, BoundType::EXACT, -1, -1);
    return std::make_tuple(value, -1, -1);
  }

  MoveList moves = genBestMoves(next, frontierRadius);
  if (moves.size == 0) {
    transpositionTable.store(key, depth, 0, BoundType::EXACT, -1, -1);
    return std::make_tuple(0, -1, -1);
  }

  if (tableX >= 0 && tableY >= 0) {
    const auto tableMove = std::find_if(
        moves.moves.begin(), moves.moves.begin() + static_cast<std::ptrdiff_t>(moves.size),
        [tableX, tableY](const ScoredMove& move) { return move.x == tableX && move.y == tableY; });
    if (tableMove != moves.moves.begin() + static_cast<std::ptrdiff_t>(moves.size)) {
      std::iter_swap(moves.moves.begin(), tableMove);
    }
  }

  int bestX = -1;
  int bestY = -1;
  Score bestValue = -kInfinity;
  for (std::size_t moveIndex = 0; moveIndex < moves.size; ++moveIndex) {
    const int x = moves.moves[moveIndex].x;
    const int y = moves.moves[moveIndex].y;
    setPieceInternal(x, y, next);
    const auto nextScore =
        negaMax(depth - 1, -beta, -alpha, otherPlayer(next), x, y, frontierRadius);
    const Score value = -std::get<0>(nextScore);
    setPieceInternal(x, y, Piece::EMPTY);

    if (value > bestValue) {
      bestX = x;
      bestY = y;
      bestValue = value;
    }
    alpha = std::max(alpha, value);
    if (beta <= alpha) {
      break;
    }
  }

  const BoundType bound = bestValue <= originalAlpha  ? BoundType::UPPER
                          : bestValue >= originalBeta ? BoundType::LOWER
                                                      : BoundType::EXACT;
  transpositionTable.store(key, depth, bestValue, bound, bestX, bestY);
  return std::make_tuple(bestValue, bestX, bestY);
}

Piece Gomoku::checkWinner() const
{
  for (const LineState& line : lines) {
    if (containsFive(line.black)) {
      return Piece::BLACK;
    }
    if (containsFive(line.white)) {
      return Piece::WHITE;
    }
  }
  return Piece::EMPTY;
}

int Gomoku::singlePieceWinner(int x, int y) const
{
  const Piece piece = board.getPiece(x, y);
  if (piece == Piece::EMPTY) {
    return static_cast<int>(Piece::EMPTY);
  }

  const CellGeometry& cell = lineGeometry().cells[static_cast<std::size_t>(x * kBoardSize + y)];
  for (int referenceIndex = 0; referenceIndex < cell.count; ++referenceIndex) {
    const CellLineReference& reference = cell.references[static_cast<std::size_t>(referenceIndex)];
    const LineState& line = lines[reference.line];
    const std::uint16_t mask = piece == Piece::BLACK ? line.black : line.white;
    int count = 1;
    for (std::uint16_t bit = static_cast<std::uint16_t>(reference.bit << 1);
         bit != 0 && (mask & bit) != 0; bit = static_cast<std::uint16_t>(bit << 1)) {
      ++count;
    }
    for (std::uint16_t bit = static_cast<std::uint16_t>(reference.bit >> 1);
         bit != 0 && (mask & bit) != 0; bit = static_cast<std::uint16_t>(bit >> 1)) {
      ++count;
    }
    if (count >= 5) {
      return static_cast<int>(piece);
    }
  }
  return static_cast<int>(Piece::EMPTY);
}

std::ostream& operator<<(std::ostream& stream, const Gomoku& gomoku)
{
  stream << "Player " << static_cast<int>(gomoku.turn) << "'s turn\n";
  stream << gomoku.board << '\n';
  return stream;
}

}  // namespace gomoku
