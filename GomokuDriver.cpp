#include "Gomoku.h"
#include "RowEvaluator.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

Gomoku *g;

void printBoard() {

#if defined(_WIN32)
  system("cls");
#else
  system("clear");
#endif
  std::cout << *g << std::endl;
}

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "need paths to row eval results" << std::endl;
    return 0;
  }

  std::vector<int> patternEvals1;
  std::vector<int> patternEvals2;

  RowEvaluator rowEvaluator;
  rowEvaluator.setPatterns(argv[1], patternEvals1, patternEvals2);

  g = new Gomoku(patternEvals1, patternEvals2);
  printBoard();

  /*
  int x, y;
  while (std::cin>>x>>y) {
          if (g->placePiece(x, y)) {
                  printBoard();
                  if (g->checkWinner()) {
                          break;
                  }
                  std::cout << "AI thinking, please wait..." << std::endl;
                  g->placePiece();
                  printBoard();
                  if (g->checkWinner()) {
                          break;
                  }
          }
          else {
                  std::cout << "Already occupied, try again" << std::endl;
          }
  }*/
  int steps = 0;
  while (!g->checkWinner()) {
    // step 4 goes first vs step 5 going second
    if ((steps % 2) == 0) {
      g->placePiece(5, true);
    }
    else {
      g->placePiece(4, true);
    }
    steps++;
    printBoard();
  }

  delete g;
  std::cout << "Press anykey to exit";
  std::cin.get(); // the enter key from previous input
  std::cin.get();
}