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
