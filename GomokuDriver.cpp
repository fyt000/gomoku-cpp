#include <cstdlib>
#include <exception>
#include <iostream>

#include "CompiledEvaluationTables.h"
#include "Gomoku.h"
#include "RowEvaluator.h"

namespace gomoku::driver {

void printBoard(const Gomoku& game)
{
#if defined(_WIN32)
  std::system("cls");
#else
  std::system("clear");
#endif
  std::cout << game << '\n';
}

int run(int argc, char** argv)
{
  if (argc > 2) {
    std::cerr << "Usage: gomoku-cpp [pattern-file]\n";
    return 1;
  }

  const EvaluationTablesPtr evaluationTables =
      argc == 2 ? RowEvaluator{}.loadPatterns(argv[1]) : compiledEvaluationTables();

  Gomoku game(evaluationTables);
  printBoard(game);

  while (game.checkWinner() == Piece::EMPTY) {
    (void)game.placePiece();
    printBoard(game);
  }

  std::cout << "Press Enter to exit.";
  std::cin.get();
  return 0;
}

}  // namespace gomoku::driver

int main(int argc, char** argv)
{
  try {
    return gomoku::driver::run(argc, argv);
  } catch (const std::exception& error) {
    std::cerr << "Error: " << error.what() << '\n';
    return 1;
  }
}
