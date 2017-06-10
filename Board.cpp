#pragma once
#include "Board.h"

Board::Board() {

}
void Board::placePiece(int x, int y, int p)
{
	if (p < 0 || p > 2)
		return;
	board[x][y] = p;
}

std::ostream & operator<<(std::ostream & stream, const Board & board)
{
	stream << " ";
	for (int j = 0; j < 15; j++) {
		stream.width(3);
		stream << j;
	}
	stream << std::endl;
	for (int i = 0; i < 15; i++) {
		stream.width(2);
		stream << i;
		for (int j = 0; j < 15; j++) {
			switch (board.board[i][j]) {
			case 0:stream << " - "; break;
			case 1:stream << " X "; break;
			case 2:stream << " O "; break;
			}
		}
		stream << std::endl;
	}

	return stream;
}