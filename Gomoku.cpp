#include "Gomoku.h"
#include "BitRowBuilder.h"
#include <algorithm>

Gomoku::Gomoku()
{
}

Gomoku::Gomoku(const std::vector<int>& patternLookup1, const std::vector<int>& patternLookup2): 
	patternLookup1(patternLookup1), patternLookup2(patternLookup2)
{
}




bool Gomoku::placePiece(int x, int y)
{
	if (board.getPiece(x, y) != Piece::EMPTY)
		return false;
	board.placePiece(x, y, turn);
	turn = otherPlayer(turn);
	return true;
}

std::pair<int,int> Gomoku::placePiece()
{
	auto p = alphaBeta(3, -99999999, 99999999, true, turn);
	int x = std::get<1>(p);
	int y = std::get<2>(p);
	//lost already
	if (x == -1 && y == -1) {
		auto anyP = genBestMoves(turn)[0];
		x = std::get<1>(anyP);
		y = std::get<2>(anyP);
	}
	placePiece(x, y);
	return { x,y };
}


int Gomoku::evalBoard(Piece player, bool isOddStep) {
	int val = 0;

	//for better locality
	//store these directions each in an array
	//but also find a mapping from placing a piece x,y
	//to update the board
	
	for (int i = 0; i < BOARDSIZE; i++) {
		//vertical
		val += rowEval(0, i, 1, 0, player, isOddStep);
		//horizontal
		val += rowEval(i, 0, 0, 1, player, isOddStep);

		//diagonal '\'
		val += rowEval(0, i, 1, 1, player, isOddStep);

		//diagonal /
		val += rowEval(0, i, 1, -1, player, isOddStep);
	}

	//the otherway for diagnoals
	for (int j = 1; j < BOARDSIZE; j++) {
		val += rowEval(j, 0, 1, 1, player, isOddStep);
		val += rowEval(j, BOARDSIZE - 1, 1, -1, player, isOddStep);
	}

	return val;
}

int Gomoku::rowEval(int x, int y, int dx, int dy, Piece self, bool isOddStep)
{
	Piece opponent = otherPlayer(self);
	BitRowBuilder rowBuilder;
	int val = 0;
	for (int i = 0; i < BOARDSIZE; i++) {
		if (x < 0 || x >= BOARDSIZE || y < 0 || y >= BOARDSIZE)
			break;
		Piece p = board.getPiece(x, y);
		x += dx;
		y += dy;
		if (p == opponent) {
			val += subRowEval(rowBuilder.getRow(), isOddStep);
			rowBuilder.reset();
			continue;
		}
		rowBuilder.add(p == Piece::EMPTY);
	}
	val += subRowEval(rowBuilder.getRow(), isOddStep);
	return val;
}

int Gomoku::subRowEval(int subRow, bool isOddStep)
{
	//less than 5
	if (subRow <= (1 << 5))
		return 0;

	if (isOddStep) {
		return patternLookup1[subRow];
	}
	else {
		return patternLookup2[subRow];
	}
}

Piece Gomoku::otherPlayer(Piece p)
{
	return p == Piece::WHITE ? Piece::BLACK : Piece::WHITE;
}


//this method still needs some tunning
//let me try to optimize first
//then I could probably allow a larger set of moves
std::vector<Gomoku::ScoreXY> Gomoku::genBestMoves(Piece cur)
{
	auto opponent = otherPlayer(cur);
	std::vector<ScoreXY> scores;
	int minX = BOARDSIZE / 2;
	int maxX = BOARDSIZE / 2 + 1;
	int minY = BOARDSIZE / 2;
	int maxY = BOARDSIZE / 2 + 1;
	for (int x = 0; x < BOARDSIZE; x++) {
		for (int y = 0; y < BOARDSIZE; y++) {
			auto p = board.getPiece(x, y);
			if (p != Piece::EMPTY) {
				minX = std::min(x, minX);
				minY = std::min(y, minY);
				maxX = std::max(x, maxX);
				maxY = std::max(y, maxY);
			}
		}
	}
	minX -= 2;
	maxX += 2;
	minY -= 2;
	maxY += 2;
	minX = std::max(0, minX);
	maxX = std::min(BOARDSIZE - 1, maxX);
	minY = std::max(0, minY);
	maxY = std::min(BOARDSIZE - 1, maxY);

	for (int x = minX; x <= maxX; x++) {
		for (int y = minY; y <= maxY; y++) {
			auto p = board.getPiece(x, y);
			if (p == Piece::EMPTY) {
				board.placePiece(x, y, cur);
				int curScore = evalBoard(cur, true);
				board.placePiece(x, y, opponent);
				curScore += evalBoard(opponent, true);
				board.placePiece(x, y, Piece::EMPTY);
				scores.emplace_back(curScore, x, y);
				/*
				board.placePiece(x, y, cur);
				int curScore = evalBoard(cur, true);
				curScore -= evalBoard(opponent, true);
				board.placePiece(x, y, opponent);
				curScore += evalBoard(opponent, true);
				scores.emplace_back(curScore, x, y);
				board.placePiece(x, y, Piece::EMPTY);
				*/
			}
		}
	}
	//sorting still helps
	std::sort(scores.begin(), scores.end(), [](const ScoreXY& lhs, const ScoreXY& rhs) {
		return std::get<0>(lhs) > std::get<0>(rhs);
	});

	//keep top 20 scores	
	//scores.resize(20);

	return scores;
}


Gomoku::ScoreXY Gomoku::alphaBeta(int depth, int alpha, int beta, bool maximizing, Piece cur)
{
	auto opponent = otherPlayer(cur);
	bool won = checkWinner();
	if (won) {
		return std::make_tuple( evalBoard(cur,true) - evalBoard(opponent,false),-1,-1 );
	}

	if (depth == 0) {
		return std::make_tuple( evalBoard(cur,true) - evalBoard(opponent,false),-1,-1 );
	}

	int bestX = -1;
	int bestY = -1;
	int bestVal = 0;

	if (maximizing) {
		bestVal = -99999999;
		for (const auto& scoreXY : genBestMoves(cur)) {
			int x = std::get<1>(scoreXY);
			int y = std::get<2>(scoreXY);
			board.placePiece(x,y,cur);
			auto nextScoreXY = alphaBeta(depth - 1, alpha, beta, false, cur);
			int v = std::get<0>(nextScoreXY);
			if (v > bestVal) {
				bestX = x;
				bestY = y;
				bestVal = v;
			}
			board.placePiece(x, y, Piece::EMPTY);
			alpha = std::max(alpha, bestVal);
			if (beta <= alpha)
				break;
		}
	}
	else {
		bestVal = 99999999;
		for (const auto& scoreXY : genBestMoves(opponent)) {
			int x = std::get<1>(scoreXY);
			int y = std::get<2>(scoreXY);
			board.placePiece(x, y, opponent);
			auto nextScoreXY = alphaBeta(depth - 1, alpha, beta, true, cur);
			int v = std::get<0>(nextScoreXY);
			if (v < bestVal) {
				bestX = x;
				bestY = y;
				bestVal = v;
			}
			board.placePiece(x, y, Piece::EMPTY);
			beta = std::min(beta, bestVal);
			if (beta <= alpha)
				break;
		}
	}
	return std::make_tuple( bestVal,bestX,bestY );
}

int Gomoku::checkWinner()
{
	int dirx[] = { 0, 1, 1, 1, 0, -1, -1, -1 };
	int diry[] = { -1, -1, 0, 1, 1, 1, 0, -1 };
	
	for (int x = 0; x <= BOARDSIZE; x++) {
		for (int y = 0; y <= BOARDSIZE; y++) {
			auto p = board.getPiece(x, y);
			if (p == Piece::EMPTY) {
				continue;
			}
			//how do I zip dirx diry
			for (int d = 0; d < 8;d++) {
				int count = 0;
				int nx = x;
				int ny = y;
				for (int i = 0; i <= 4; i++) {
					nx = nx + dirx[d];
					ny = ny + diry[d];
					if (nx >= 0 && nx < 15 && ny >= 0 && ny < 15) {
						if (board.getPiece(nx, ny) != p) {
							break;
						}
						else {
							count++;
						}
					}
					else
						break;
				}
				if (count == 4)
					return (int)p;
			}
		}
	}
	return 0;
}


std::ostream & operator<<(std::ostream & stream, const Gomoku & gomoku)
{
	stream << "Player " << gomoku.turn << "'s turn" << std::endl;
	stream << gomoku.board << std::endl;
	return stream;
}

