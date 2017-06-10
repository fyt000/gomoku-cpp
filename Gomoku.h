#pragma once
#include <iostream>
#include "Board.h"

class Gomoku {
public:
	Gomoku();
	void placePiece(int x,int y);
	friend std::ostream& operator<< (std::ostream& stream, const Gomoku& gomoku);

private:
	int turn = 1;
	Board board;
};