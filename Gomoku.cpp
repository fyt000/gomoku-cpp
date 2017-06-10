#include "Gomoku.h"

Gomoku::Gomoku()
{
}

void Gomoku::placePiece(int x, int y)
{
	board.placePiece(x, y, turn);
	turn = turn == 1 ? 2 : 1;
}

std::ostream & operator<<(std::ostream & stream, const Gomoku & gomoku)
{
	stream << "Player " << gomoku.turn << "'s turn" << std::endl;
	stream << gomoku.board << std::endl;
	return stream;
}

