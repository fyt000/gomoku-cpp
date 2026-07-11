#include <bit>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>

#include "BitRowBuilder.h"
#include "Board.h"
#include "GameSession.h"
#include "Gomoku.h"
#include "RowEvaluator.h"

namespace gomoku::test {

namespace {

EvaluationTablesPtr zeroTables()
{
  static const EvaluationTablesPtr tables = std::make_shared<const EvaluationTables>();
  return tables;
}

EvaluationTablesPtr syntheticTables()
{
  static const EvaluationTablesPtr tables = [] {
    auto values = std::make_shared<EvaluationTables>();
    for (std::size_t row = 0; row < kEncodedRowCount; ++row) {
      const auto encoded = static_cast<std::uint16_t>(row);
      values->oddStep[row] = static_cast<std::int32_t>(std::popcount(encoded) * 17);
      values->evenStep[row] = static_cast<std::int32_t>(std::popcount(encoded) * 13);
    }
    return values;
  }();
  return tables;
}

bool bitboardContains(const Board::Bitboard& bits, int row, int column)
{
  const int index = row * kBoardStride + column;
  return (bits[static_cast<std::size_t>(index / 64)] & (std::uint64_t{1} << (index % 64))) != 0;
}

int bitboardCount(const Board::Bitboard& bits)
{
  int count = 0;
  for (const std::uint64_t word : bits) {
    count += static_cast<int>(std::popcount(word));
  }
  return count;
}

Piece naiveWinner(const Piece (&cells)[kBoardSize][kBoardSize])
{
  for (int row = 0; row < kBoardSize; ++row) {
    for (int column = 0; column < kBoardSize; ++column) {
      const Piece piece = cells[row][column];
      if (piece == Piece::EMPTY) {
        continue;
      }
      for (const auto [deltaRow, deltaColumn] :
           {std::pair{0, 1}, std::pair{1, 0}, std::pair{1, 1}, std::pair{1, -1}}) {
        int count = 1;
        for (const int sign : {-1, 1}) {
          for (int offset = 1; offset < 5; ++offset) {
            const int nextRow = row + sign * offset * deltaRow;
            const int nextColumn = column + sign * offset * deltaColumn;
            if (nextRow < 0 || nextRow >= kBoardSize || nextColumn < 0 ||
                nextColumn >= kBoardSize || cells[nextRow][nextColumn] != piece) {
              break;
            }
            ++count;
          }
        }
        if (count >= 5) {
          return piece;
        }
      }
    }
  }
  return Piece::EMPTY;
}

}  // namespace

TEST_CASE("Board starts empty and stores valid pieces", "[board]")
{
  Board board;

  CHECK(board.getPiece(7, 7) == Piece::EMPTY);

  board.placePiece(7, 7, Piece::BLACK);
  CHECK(board.getPiece(7, 7) == Piece::BLACK);

  board.placePiece(7, 7, static_cast<Piece>(99));
  CHECK(board.getPiece(7, 7) == Piece::BLACK);
  CHECK(board.pieceCount() == 1);
  CHECK_FALSE(board.isEmpty());
  CHECK(sizeof(Piece) == 1);
  CHECK(sizeof(Board) == 64);
}

TEST_CASE("Board frontier stays within two intersections without wrapping", "[board][frontier]")
{
  Board board;
  board.placePiece(0, 14, Piece::BLACK);

  const Board::Bitboard frontier = board.frontier(2);
  CHECK(bitboardCount(frontier) == 8);
  CHECK(bitboardContains(frontier, 0, 12));
  CHECK(bitboardContains(frontier, 2, 14));
  CHECK(bitboardContains(frontier, 2, 12));
  CHECK_FALSE(bitboardContains(frontier, 1, 0));
  CHECK_FALSE(bitboardContains(frontier, 0, 14));
}

TEST_CASE("BitRowBuilder tracks row data and length", "[bit-row]")
{
  BitRowBuilder row;
  row.appendCell(true).appendCell(false).appendCell(false);

  CHECK(row.length() == 3);
  CHECK(row.getRow() == 0b1100);
  CHECK(row.isSet(0));
  CHECK_FALSE(row.isSet(1));
  CHECK_FALSE(row.isSet(2));
  CHECK(BitRowBuilder::GetReverse(row.getRow()) == 0b1001);
}

TEST_CASE("BitRowBuilder operations hold for every encoded row", "[bit-row]")
{
  bool valid = true;
  for (std::uint32_t encoded = 1; encoded < kEncodedRowCount && valid; ++encoded) {
    const auto row = static_cast<std::uint16_t>(encoded);
    const int length = static_cast<int>(std::bit_width(row)) - 1;
    valid = BitRowBuilder::LengthOf(row) == length &&
            BitRowBuilder::GetReverse(BitRowBuilder::GetReverse(row)) == row;
    for (int removed = 0; removed <= length && valid; ++removed) {
      const int remaining = length - removed;
      const std::uint16_t lowMask =
          remaining == 0 ? 0 : static_cast<std::uint16_t>((1u << remaining) - 1u);
      const auto expected =
          static_cast<std::uint16_t>((row & lowMask) | (std::uint16_t{1} << remaining));
      valid = BitRowBuilder::RowSplice(row, removed) == expected;
    }
  }
  CHECK(valid);
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

TEST_CASE("Gomoku detects all four winning directions", "[gomoku][winner]")
{
  for (const auto [deltaRow, deltaColumn] :
       {std::pair{0, 1}, std::pair{1, 0}, std::pair{1, 1}, std::pair{1, -1}}) {
    Piece cells[kBoardSize][kBoardSize] = {};
    const int startRow = 4;
    const int startColumn = deltaColumn < 0 ? 9 : 4;
    for (int offset = 0; offset < 5; ++offset) {
      cells[startRow + offset * deltaRow][startColumn + offset * deltaColumn] = Piece::WHITE;
    }

    Gomoku game;
    game.setBoard(cells);
    CHECK(game.checkWinner() == Piece::WHITE);
  }
}

TEST_CASE("Incremental winner state matches a naive board scan", "[gomoku][winner]")
{
  for (int sequence = 0; sequence < 20; ++sequence) {
    Piece cells[kBoardSize][kBoardSize] = {};
    Gomoku game;

    for (int move = 0; move < 60; ++move) {
      const int index = (sequence * 37 + move * 73) % (kBoardSize * kBoardSize);
      const int row = index / kBoardSize;
      const int column = index % kBoardSize;
      const Piece piece = game.currentTurn();
      REQUIRE(game.placePiece(row, column));
      cells[row][column] = piece;
      CHECK(game.checkWinner() == naiveWinner(cells));
      if (game.checkWinner() != Piece::EMPTY) {
        break;
      }
    }
  }
}

TEST_CASE("An empty imported board has Black to move", "[gomoku]")
{
  Piece cells[kBoardSize][kBoardSize] = {};
  Gomoku game;
  game.setBoard(cells);

  CHECK(game.currentTurn() == Piece::BLACK);
}

TEST_CASE("Default Gomoku tables are safe for computer search", "[gomoku][engine]")
{
  Gomoku game;
  REQUIRE(game.placePiece(7, 7));

  const auto [row, column] = game.placePiece();
  CHECK(row >= 0);
  CHECK(column >= 0);
  CHECK(game.getPiece(row, column) == Piece::WHITE);
}

TEST_CASE("RowEvaluator rejects patterns longer than the board", "[row-evaluator]")
{
  const std::filesystem::path path =
      std::filesystem::temp_directory_path() / "gomoku-invalid-pattern.txt";
  {
    std::ofstream output(path);
    output << "0101010101010101 1 1\n";
  }

  RowEvaluator evaluator;
  CHECK_THROWS_AS((void)evaluator.loadPatterns(path.string()), std::runtime_error);
  std::filesystem::remove(path);
}

TEST_CASE("Computer opens at the centre", "[gomoku][engine]")
{
  Gomoku game(zeroTables());

  CHECK(game.placePiece() == std::pair{7, 7});
  CHECK(game.getPiece(7, 7) == Piece::BLACK);
  CHECK(game.currentTurn() == Piece::WHITE);
}

TEST_CASE("Computer takes an immediate five", "[gomoku][engine]")
{
  Piece cells[kBoardSize][kBoardSize] = {};
  for (int column = 4; column < 8; ++column) {
    cells[7][column] = Piece::BLACK;
  }
  cells[6][4] = Piece::WHITE;
  cells[6][5] = Piece::WHITE;
  cells[6][7] = Piece::WHITE;
  cells[8][6] = Piece::WHITE;

  Gomoku game(zeroTables());
  game.setBoard(cells);

  const auto [x, y] = game.placePiece();
  CHECK(x == 7);
  CHECK((y == 3 || y == 8));
  CHECK(game.checkWinner() == Piece::BLACK);
}

TEST_CASE("Computer blocks an opponent's immediate five", "[gomoku][engine]")
{
  Piece cells[kBoardSize][kBoardSize] = {};
  for (int column = 4; column < 8; ++column) {
    cells[7][column] = Piece::BLACK;
  }
  cells[7][3] = Piece::WHITE;
  cells[6][4] = Piece::WHITE;
  cells[6][5] = Piece::WHITE;

  Gomoku game(zeroTables());
  game.setBoard(cells);

  REQUIRE(game.currentTurn() == Piece::WHITE);
  const auto [x, y] = game.placePiece();
  CHECK(x == 7);
  CHECK(y == 8);
  CHECK(game.checkWinner() == Piece::EMPTY);
}

TEST_CASE("Computer blocks a five completed through an internal gap", "[gomoku][engine]")
{
  Piece cells[kBoardSize][kBoardSize] = {};

  for (const auto [row, column] :
       {std::pair{4, 6}, std::pair{5, 6}, std::pair{5, 7}, std::pair{5, 9}, std::pair{5, 10},
        std::pair{6, 5}, std::pair{6, 8}, std::pair{7, 7}, std::pair{7, 9}, std::pair{8, 4},
        std::pair{8, 9}, std::pair{10, 6}, std::pair{11, 10}}) {
    cells[row][column] = Piece::BLACK;
  }
  for (const auto [row, column] :
       {std::pair{3, 5}, std::pair{6, 6}, std::pair{7, 6}, std::pair{8, 5}, std::pair{8, 6},
        std::pair{8, 7}, std::pair{8, 8}, std::pair{8, 10}, std::pair{9, 6}, std::pair{9, 8},
        std::pair{10, 8}, std::pair{10, 9}}) {
    cells[row][column] = Piece::WHITE;
  }

  Gomoku game(zeroTables());
  game.setBoard(cells);

  REQUIRE(game.currentTurn() == Piece::WHITE);
  CHECK(game.placePiece() == std::pair{5, 8});
  CHECK(game.checkWinner() == Piece::EMPTY);
}

TEST_CASE("Parallel root search agrees with serial search", "[gomoku][engine][threading]")
{
  Piece cells[kBoardSize][kBoardSize] = {};
  for (const auto [row, column] :
       {std::pair{7, 7}, std::pair{6, 6}, std::pair{8, 8}, std::pair{6, 8}}) {
    cells[row][column] = Piece::BLACK;
  }
  for (const auto [row, column] :
       {std::pair{7, 8}, std::pair{7, 6}, std::pair{6, 7}, std::pair{8, 7}}) {
    cells[row][column] = Piece::WHITE;
  }

  Gomoku serial(syntheticTables());
  Gomoku parallel(syntheticTables());
  serial.setBoard(cells);
  parallel.setBoard(cells);

  const auto serialMove = serial.placePiece(SearchOptions{.depth = 4, .threadCount = 1});
  const auto parallelMove = parallel.placePiece(SearchOptions{.depth = 4, .threadCount = 4});
  CHECK(parallelMove == serialMove);

  Gomoku wideSerial(syntheticTables());
  Gomoku wideParallel(syntheticTables());
  wideSerial.setBoard(cells);
  wideParallel.setBoard(cells);
  const auto wideSerialMove =
      wideSerial.placePiece(SearchOptions{.depth = 2, .frontierRadius = 3, .threadCount = 1});
  const auto wideParallelMove =
      wideParallel.placePiece(SearchOptions{.depth = 2, .frontierRadius = 3, .threadCount = 4});
  CHECK(wideParallelMove == wideSerialMove);
}

TEST_CASE("GameSession rebuilds state from moves when undoing", "[game-session]")
{
  GameSession game(zeroTables());

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
