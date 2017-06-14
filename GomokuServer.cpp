#include <cpprest/http_listener.h>
#include <cpprest/json.h>
#include "Gomoku.h"


using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;

#include <iostream>
#include <map>
#include <set>
#include <string>

using namespace std;

std::vector<int> patternEvals1;
std::vector<int> patternEvals2;

void readPatternEvalDump(std::string filePath, std::vector<int>& patternEvals) {
	std::ifstream fin(filePath.c_str()); //check if I can actually just pass a string now 
	int eval;
	while (fin >> eval) {
		patternEvals.push_back(eval);
	}
}

void isWinnerCheck(http_request request)
{
	cerr << "receiving post request" << endl;
	Gomoku g(patternEvals1, patternEvals2);
	int result = 0;
	request.extract_json().then([&g,&result](pplx::task<json::value> task) {
		//I hate json and every json library
		//protobuf when?
		auto& jsonMap = task.get();
		auto& boardArray = jsonMap.at(L"board").as_array();
		Piece board[BOARDSIZE][BOARDSIZE];
		for (int i = 0; i < BOARDSIZE; i++) {
			for (int j = 0; j < BOARDSIZE; j++) {
				board[i][j] = (Piece) boardArray.at(i*BOARDSIZE + j).as_integer();
			}
		}
		g.setBoard(board);
		result = g.checkWinner();
	}).wait();

	auto responseJson = json::value::object();
	responseJson[L"winner"] = result;
	request.reply(status_codes::OK, responseJson);
}

void getNextStep(http_request request)
{
	cerr << "receiving getNextStep request" << endl;
	Gomoku g(patternEvals1, patternEvals2);
	pair<int, int> nextXY;
	request.extract_json().then([&g, &nextXY](pplx::task<json::value> task) {
		//I hate json and every json library
		//protobuf when?
		auto& jsonMap = task.get();
		auto& boardArray = jsonMap.at(L"board").as_array();
		Piece board[BOARDSIZE][BOARDSIZE];
		for (int i = 0; i < BOARDSIZE; i++) {
			for (int j = 0; j < BOARDSIZE; j++) {
				board[i][j] = (Piece)boardArray.at(i*BOARDSIZE + j).as_integer();
			}
		}
		g.setBoard(board);
		nextXY = g.placePiece();

	}).wait();
	auto responseJson = json::value::object();
	responseJson[L"x"] = nextXY.first;
	responseJson[L"y"] = nextXY.second;
	request.reply(status_codes::OK, responseJson);
}


int main(int argc, char** argv) {

	readPatternEvalDump(argv[1], patternEvals1);
	readPatternEvalDump(argv[2], patternEvals2);

	http_listener winnerListener(L"http://localhost:5000/api/iswinner/");
	winnerListener.support(methods::POST, isWinnerCheck);

	http_listener nextStepListener(L"http://localhost:5000/api/getnextmove/");
	nextStepListener.support(methods::POST, getNextStep);

	try
	{
		winnerListener.open();
		nextStepListener.open();
		std::cout << "Press ENTER to exit." << std::endl;

		std::string line;
		std::getline(std::cin, line);
	}
	catch (std::exception const & e)
	{
		std::cerr << "Error: " << e.what() << "\n";
	}

}