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

	template<int R, int C>
	void setBoard(Piece(&board)[R][C])
	{
		//c++11 is good
		this->board = Board(board);

		//pass in the turn value.
		int pieceCount = 0;
		for (int i = 0; i < 15;i++) {
			for (int j=0;j<15;j++){
				auto p = board[i][j];
				switch (p) {
				case Piece::WHITE :
					pieceCount -= 1; break;
				case Piece::BLACK:
					pieceCount += 1; break;
				}
			}
		}
		if (pieceCount < 0) {
			turn = Piece::BLACK;
            std::cerr<<"black going"<<std::endl;
		}
		else {
			turn = Piece::WHITE;
            std::cerr<<"white going"<<std::endl;
		}
		//std::cerr<< turn<<std::endl;
	}

	bool placePiece(int x,int y);
	std::pair<int,int> placePiece();
	int checkWinner();
	friend std::ostream& operator<< (std::ostream& stream, const Gomoku& gomoku);

private:
	// int maxScore = 0;
	// int wonScore;
	Piece turn = Piece::BLACK;
	Board board;
	const std::vector<int> patternLookup1;
	const std::vector<int> patternLookup2;
	int evalBoard(Piece player, bool isOddStep);
	int rowEval(int sx, int sy, int dx, int dy, Piece pType, bool isOddStep);
	int subRowEval(int subRow, bool isOddStep);
	Piece otherPlayer(Piece p);
	std::vector<ScoreXY> genBestMoves(Piece cur);
	ScoreXY negaMax(int depth, int alpha, int beta, Piece start, Piece next);
	int singlePieceWinner(int x, int y);
};
