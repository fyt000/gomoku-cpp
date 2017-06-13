#include "Board.h"

Board::Board() {

}


void Board::placePiece(int x, int y, Piece p)
{
	if (p < 0 || p > 2)
		return;
	board[x][y] = p;
}

Piece Board::getPiece(int x, int y)
{
	return board[x][y];
}

//I wish writing an interface could be this simple
std::ostream & operator<<(std::ostream & stream, const Board & board)
{
	stream << " ";
	for (int j = 0; j < BOARDSIZE; j++) {
		stream.width(3);
		stream << j;
	}
	stream << std::endl;
	for (int i = 0; i < BOARDSIZE; i++) {
		stream.width(2);
		stream << i;
		for (int j = 0; j < BOARDSIZE; j++) {
			switch (board.board[i][j]) {
			case Piece::EMPTY:stream << " - "; break;
			case Piece::BLACK:stream << " X "; break;
			case Piece::WHITE:stream << " O "; break;
			}
		}
		stream << std::endl;
	}

	return stream;
}
