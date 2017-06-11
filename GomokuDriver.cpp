#include <iostream>
#include <cstdlib>
#include <string>
#include "Gomoku.h"
#include <fstream>
#include <vector>

//PLAN:
//1. make AI work
//2. multithreading on top of AI
//3. protobuf/thrift for interprocess communicating (self-learn), and web interface


// focus on getting stuff done
// will refactor later

Gomoku* g;

void printBoard() {

#if defined(_WIN32)
	system("cls");
#else
	system("clear");
#endif
	std::cout << *g << std::endl;
}

void readPatternEvalDump(std::string filePath, std::vector<int>& patternEvals) {
	std::ifstream fin(filePath.c_str()); //check if I can actually just pass a string now 
	int eval;
	while (fin >> eval) {
		patternEvals.push_back(eval);
	}
}

int main(int argc, char** argv) {
	if (argc != 3) {
		std::cerr << "need paths to row eval results" << std::endl;
		return 0;
	}

	std::vector<int> patternEvals1;
	std::vector<int> patternEvals2;

	readPatternEvalDump(argv[1], patternEvals1);
	readPatternEvalDump(argv[2], patternEvals2);

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

	
	while (!g->checkWinner()) {
		g->placePiece();
		printBoard();
	}
	
	delete g;
	std::cout << "Press anykey to exit";
	std::cin.get(); //the enter key from previous input
	std::cin.get();
}