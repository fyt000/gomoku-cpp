#pragma once
#include <iostream>

const int BOARDSIZE = 15;

class Board {
public:	
	Board();
	void placePiece(int x,int y,int p);
	friend std::ostream& operator<< (std::ostream& stream, const Board& gomoku);

private:
	int zobristHash;
	bool hashDirty = true;
	//maybe store it in a way that is locality friendly
	int board[BOARDSIZE][BOARDSIZE] = { {0} };
};