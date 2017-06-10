#include <iostream>
#include <cstdlib>
#include <string>
#include "Gomoku.h"

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

int main() {
	g = new Gomoku();
	printBoard();
	//std::string line;
	//std::getline(std::cin, line)
	int x, y;
	while (std::cin>>x>>y) {
		g->placePiece(x, y);
		printBoard();
	}
}