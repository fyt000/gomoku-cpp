#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <tuple>
#include <utility>

#include "Board.h"
#include "EvaluationTables.h"

namespace gomoku {

struct SearchOptions {
  int depth = 4;
  // Candidate cells within this Chebyshev distance of existing stones.
  // Values are clamped to 1-4; radius two is the recommended default.
  int frontierRadius = 2;
  // Parallelizes root moves. Values above eight are clamped to bound the
  // memory consumed by per-thread transposition tables.
  unsigned int threadCount = 1;
};

class Gomoku {
  using Score = std::int64_t;
  using ScoreXY = std::tuple<Score, int, int>;

  static constexpr std::size_t kUsefulLineCount = 72;
  static constexpr std::size_t kMaximumMoveCount = kBoardSize * kBoardSize;

  struct LineState {
    std::uint16_t black = 0;
    std::uint16_t white = 0;
    std::array<Score, 4> scores{};
  };

  struct ScoredMove {
    Score score = 0;
    std::uint8_t x = 0;
    std::uint8_t y = 0;
  };

  struct MoveList {
    std::array<ScoredMove, kMaximumMoveCount> moves{};
    std::size_t size = 0;
  };

 public:
  Gomoku();
  explicit Gomoku(EvaluationTablesPtr evaluationTables);

  template <int Rows, int Columns>
  void setBoard(Piece (&cells)[Rows][Columns])
  {
    static_assert(Rows == kBoardSize && Columns == kBoardSize,
                  "Board dimensions must be 15 by 15.");
    board = Board(cells);

    int pieceBalance = 0;
    for (int row = 0; row < kBoardSize; ++row) {
      for (int column = 0; column < kBoardSize; ++column) {
        switch (cells[row][column]) {
          case Piece::WHITE:
            --pieceBalance;
            break;
          case Piece::BLACK:
            ++pieceBalance;
            break;
          case Piece::EMPTY:
            break;
        }
      }
    }
    turn = pieceBalance <= 0 ? Piece::BLACK : Piece::WHITE;
    rebuildDerivedState();
  }

  bool placePiece(int x, int y);
  [[nodiscard]] std::pair<int, int> placePiece();
  [[nodiscard]] std::pair<int, int> placePiece(int searchDepth);
  [[nodiscard]] std::pair<int, int> placePiece(SearchOptions options);
  void reset();
  [[nodiscard]] Piece checkWinner() const;
  [[nodiscard]] Piece getPiece(int x, int y) const;
  [[nodiscard]] Piece currentTurn() const;
  [[nodiscard]] bool isBoardFull() const;

  friend std::ostream& operator<<(std::ostream& stream, const Gomoku& gomoku);

 private:
  Piece turn = Piece::BLACK;
  Board board;
  EvaluationTablesPtr evaluationTables;
  std::array<LineState, kUsefulLineCount> lines{};
  std::array<Score, 4> totalScores{};
  std::uint64_t positionHash = 0;

  [[nodiscard]] static EvaluationTablesPtr zeroEvaluationTables();
  [[nodiscard]] static Piece otherPlayer(Piece piece);
  [[nodiscard]] static int playerIndex(Piece piece);
  [[nodiscard]] static std::size_t scoreIndex(Piece piece, bool isOddStep);
  [[nodiscard]] Score subRowEval(int subRow, bool isOddStep) const;
  void recomputeLine(std::size_t lineIndex);
  void rebuildDerivedState();
  void setPieceInternal(int x, int y, Piece piece);
  [[nodiscard]] Score totalScore(Piece player, bool isOddStep) const;
  [[nodiscard]] MoveList genBestMoves(Piece current, int frontierRadius);
  [[nodiscard]] ScoreXY parallelRootSearch(int depth, Piece next, int frontierRadius,
                                           unsigned int threadCount);
  [[nodiscard]] ScoreXY negaMax(int depth, Score alpha, Score beta, Piece next, int lastX,
                                int lastY, int frontierRadius);
  [[nodiscard]] int singlePieceWinner(int x, int y) const;
};

}  // namespace gomoku
