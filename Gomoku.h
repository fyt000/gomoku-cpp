#pragma once
#include <iostream>
#include "Board.h"
#include <vector>
#include <tuple>

// not implementing score/weight lookup...
// will add the other script that does it
class Gomoku {
	typedef std::tuple<int, int, int> ScoreXY;
public:
	Gomoku();
	Gomoku(const std::vector<int> &patternLookup1, const std::vector<int> &patternLookup2);
	bool placePiece(int x,int y);
	void placePiece();
	bool checkWinner();
	friend std::ostream& operator<< (std::ostream& stream, const Gomoku& gomoku);

private:
	Piece turn = Piece::BLACK;
	Board board;
	const std::vector<int> patternLookup1;
	const std::vector<int> patternLookup2;
	int evalBoard(Piece player, bool isOddStep);
	int rowEval(int sx, int sy, int dx, int dy, Piece pType, bool isOddStep);
	int subRowEval(int subRow, bool isOddStep);
	Piece otherPlayer(Piece p);
	std::vector<ScoreXY> genBestMoves(Piece cur);
	ScoreXY alphaBeta(int depth, int alpha, int beta, bool maximizing, Piece self);
};