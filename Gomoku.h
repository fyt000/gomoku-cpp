#pragma once

#include <iosfwd>
#include <tuple>
#include <utility>
#include <vector>

#include "Board.h"

namespace gomoku {

class Gomoku {
  using ScoreXY = std::tuple<int, int, int>;

  struct SearchBounds {
    int minX;
    int maxX;
    int minY;
    int maxY;
  };

 public:
  Gomoku();
  Gomoku(const std::vector<int>& patternLookup1, const std::vector<int>& patternLookup2);

  template <int Rows, int Columns>
  void setBoard(Piece (&cells)[Rows][Columns])
  {
    board = Board(cells);

    int pieceCount = 0;
    for (int row = 0; row < kBoardSize; ++row) {
      for (int column = 0; column < kBoardSize; ++column) {
        switch (cells[row][column]) {
          case Piece::WHITE:
            --pieceCount;
            break;
          case Piece::BLACK:
            ++pieceCount;
            break;
          case Piece::EMPTY:
            break;
        }
      }
    }
    turn = pieceCount <= 0 ? Piece::BLACK : Piece::WHITE;
  }

  bool placePiece(int x, int y);
  [[nodiscard]] std::pair<int, int> placePiece();
  void reset();
  [[nodiscard]] int checkWinner() const;
  [[nodiscard]] Piece getPiece(int x, int y) const;
  [[nodiscard]] Piece currentTurn() const;
  [[nodiscard]] bool isBoardFull() const;

  friend std::ostream& operator<<(std::ostream& stream, const Gomoku& gomoku);

 private:
  Piece turn = Piece::BLACK;
  Board board;
  const std::vector<int> patternLookup1;
  const std::vector<int> patternLookup2;

  [[nodiscard]] int evalBoard(Piece player, bool isOddStep);
  [[nodiscard]] int rowEval(int startX, int startY, int deltaX, int deltaY, Piece piece,
                            bool isOddStep);
  [[nodiscard]] int subRowEval(int subRow, bool isOddStep);
  [[nodiscard]] int singlePieceEvaluation(int x, int y, Piece player, bool isOddStep);
  [[nodiscard]] Piece otherPlayer(Piece piece) const;
  [[nodiscard]] bool isBoardEmpty() const;
  [[nodiscard]] SearchBounds findSearchBounds() const;
  [[nodiscard]] static SearchBounds includeMove(SearchBounds bounds, int x, int y);
  [[nodiscard]] std::vector<ScoreXY> genBestMoves(Piece current, SearchBounds bounds);
  [[nodiscard]] ScoreXY negaMax(int depth, int alpha, int beta, Piece next, int lastX,
                                 int lastY, SearchBounds bounds);
  [[nodiscard]] int singlePieceWinner(int x, int y) const;
};

}  // namespace gomoku
