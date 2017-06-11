#pragma once
#include <iostream>

const int BOARDSIZE = 15;

enum Piece {
	EMPTY,
	BLACK,
	WHITE
};


class Board {
public:	
	Board();
	void placePiece(int x,int y,Piece p);
	Piece getPiece(int x, int y);
	friend std::ostream& operator<< (std::ostream& stream, const Board& gomoku);

private:
	//TODO implement zobristHashing and transposition
	int zobristHash;
	bool hashDirty = true;
	//maybe store it in a way that is locality friendly
	Piece board[BOARDSIZE][BOARDSIZE] = { {Piece::EMPTY} };
};
