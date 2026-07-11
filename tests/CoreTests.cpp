#include <catch2/catch_test_macros.hpp>
#include <vector>

#include "BitRowBuilder.h"
#include "Board.h"
#include "GameSession.h"
#include "Gomoku.h"

namespace gomoku::test {

TEST_CASE("Board starts empty and stores valid pieces", "[board]")
{
  Board board;

  CHECK(board.getPiece(7, 7) == Piece::EMPTY);

  board.placePiece(7, 7, Piece::BLACK);
  CHECK(board.getPiece(7, 7) == Piece::BLACK);

  board.placePiece(7, 7, static_cast<Piece>(99));
  CHECK(board.getPiece(7, 7) == Piece::BLACK);
}

TEST_CASE("BitRowBuilder tracks row data and length", "[bit-row]")
{
  BitRowBuilder row;
  row.add(false).add(true).add(true);

  CHECK(row.length() == 3);
  CHECK(row.getRow() == 0b1100);
  CHECK(row.isSet(0));
  CHECK_FALSE(row.isSet(1));
  CHECK_FALSE(row.isSet(2));
  CHECK(BitRowBuilder::GetReverse(row.getRow()) == 0b1001);
}

TEST_CASE("Gomoku detects a five-piece row", "[gomoku]")
{
  Piece cells[kBoardSize][kBoardSize] = {};
  for (int column = 0; column < 5; ++column) {
    cells[4][column] = Piece::BLACK;
  }

  Gomoku game;
  game.setBoard(cells);

  CHECK(game.checkWinner() == Piece::BLACK);
}

TEST_CASE("An empty imported board has Black to move", "[gomoku]")
{
  Piece cells[kBoardSize][kBoardSize] = {};
  Gomoku game;
  game.setBoard(cells);

  CHECK(game.currentTurn() == Piece::BLACK);
}

TEST_CASE("Computer opens at the centre", "[gomoku][engine]")
{
  const std::vector<int> patternLookup(1 << 16);
  Gomoku game(patternLookup, patternLookup);

  CHECK(game.placePiece() == std::pair{7, 7});
  CHECK(game.getPiece(7, 7) == Piece::BLACK);
  CHECK(game.currentTurn() == Piece::WHITE);
}

TEST_CASE("Computer takes an immediate five", "[gomoku][engine]")
{
  const std::vector<int> patternLookup(1 << 16);
  Piece cells[kBoardSize][kBoardSize] = {};
  for (int column = 4; column < 8; ++column) {
    cells[7][column] = Piece::BLACK;
  }
  cells[6][4] = Piece::WHITE;
  cells[6][5] = Piece::WHITE;
  cells[6][7] = Piece::WHITE;
  cells[8][6] = Piece::WHITE;

  Gomoku game(patternLookup, patternLookup);
  game.setBoard(cells);

  const auto [x, y] = game.placePiece();
  CHECK(x == 7);
  CHECK((y == 3 || y == 8));
  CHECK(game.checkWinner() == Piece::BLACK);
}

TEST_CASE("Computer blocks an opponent's immediate five", "[gomoku][engine]")
{
  const std::vector<int> patternLookup(1 << 16);
  Piece cells[kBoardSize][kBoardSize] = {};
  for (int column = 4; column < 8; ++column) {
    cells[7][column] = Piece::BLACK;
  }
  cells[7][3] = Piece::WHITE;
  cells[6][4] = Piece::WHITE;
  cells[6][5] = Piece::WHITE;

  Gomoku game(patternLookup, patternLookup);
  game.setBoard(cells);

  REQUIRE(game.currentTurn() == Piece::WHITE);
  const auto [x, y] = game.placePiece();
  CHECK(x == 7);
  CHECK(y == 8);
  CHECK(game.checkWinner() == Piece::EMPTY);
}

TEST_CASE("Computer blocks a five completed through an internal gap", "[gomoku][engine]")
{
  const std::vector<int> patternLookup(1 << 16);
  Piece cells[kBoardSize][kBoardSize] = {};

  for (const auto [row, column] : {std::pair{4, 6}, std::pair{5, 6}, std::pair{5, 7},
                                   std::pair{5, 9}, std::pair{5, 10}, std::pair{6, 5},
                                   std::pair{6, 8}, std::pair{7, 7}, std::pair{7, 9},
                                   std::pair{8, 4}, std::pair{8, 9}, std::pair{10, 6},
                                   std::pair{11, 10}}) {
    cells[row][column] = Piece::BLACK;
  }
  for (const auto [row, column] : {std::pair{3, 5}, std::pair{6, 6}, std::pair{7, 6},
                                   std::pair{8, 5}, std::pair{8, 6}, std::pair{8, 7},
                                   std::pair{8, 8}, std::pair{8, 10}, std::pair{9, 6},
                                   std::pair{9, 8}, std::pair{10, 8}, std::pair{10, 9}}) {
    cells[row][column] = Piece::WHITE;
  }

  Gomoku game(patternLookup, patternLookup);
  game.setBoard(cells);

  REQUIRE(game.currentTurn() == Piece::WHITE);
  CHECK(game.placePiece() == std::pair{5, 8});
  CHECK(game.checkWinner() == Piece::EMPTY);
}

TEST_CASE("GameSession rebuilds state from moves when undoing", "[game-session]")
{
  const std::vector<int> patternLookup(1 << 16);
  GameSession game(patternLookup, patternLookup);

  CHECK(game.currentTurn() == Piece::BLACK);
  CHECK(game.placePiece(7, 7));
  CHECK(game.currentTurn() == Piece::WHITE);
  CHECK(game.moveCount() == 1);
  CHECK(game.undo() == 1);
  CHECK(game.getPiece(7, 7) == Piece::EMPTY);
  CHECK(game.currentTurn() == Piece::BLACK);
  CHECK(game.undo() == 0);
}

}  // namespace gomoku::test
