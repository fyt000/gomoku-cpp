#include <cpprest/http_listener.h>
#include <cpprest/json.h>
#include <cpprest/uri.h>
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

void defaultOption(http_request request)
{
	http_response response(status_codes::OK);
	response.headers().add(U("Allow"), U("POST, OPTIONS"));
	response.headers().add(U("Access-Control-Allow-Origin"), U("*"));
	response.headers().add(U("Access-Control-Allow-Methods"), U("POST, OPTIONS"));
	response.headers().add(U("Access-Control-Allow-Headers"), U("Content-Type"));
	request.reply(response);
}


void isWinnerCheck(http_request request)
{
	cerr << "receiving post request" << endl;
	Gomoku g(patternEvals1, patternEvals2);
	int result = 0;
	request.extract_json().then([&g,&result](pplx::task<json::value> task) {
			//I hate json and every json library
			//protobuf when?
			const auto& jsonMap = task.get();
			auto& boardArray = jsonMap.at(utility::conversions::to_utf8string("board")).as_array();
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
	responseJson[utility::conversions::to_utf8string("winner")] = result;
	http_response response(status_codes::OK);
	response.headers().add(U("Access-Control-Allow-Origin"), U("*"));
	response.set_body(responseJson);
	request.reply(response);
}

void getNextStep(http_request request)
{
	cerr << "receiving getNextStep request" << endl;
	Gomoku g(patternEvals1, patternEvals2);
	pair<int, int> nextXY;
	request.extract_json().then([&g, &nextXY](pplx::task<json::value> task) {
			//I hate json and every json library
			//protobuf when?
			const auto& jsonMap = task.get();
			auto& boardArray = jsonMap.at(utility::conversions::to_utf8string("board")).as_array();
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
	responseJson[utility::conversions::to_utf8string("x")] = nextXY.first;
	responseJson[utility::conversions::to_utf8string("y")] = nextXY.second;
	http_response response(status_codes::OK);
	response.headers().add(U("Access-Control-Allow-Origin"), U("*"));
	response.set_body(responseJson);
	request.reply(response);
}




int main(int argc, char** argv) {

	readPatternEvalDump(argv[1], patternEvals1);
	readPatternEvalDump(argv[2], patternEvals2);

	http_listener winnerListener(utility::conversions::to_utf8string("http://localhost:5000/api/iswinner/"));
	winnerListener.support(methods::POST, isWinnerCheck);
	winnerListener.support(methods::OPTIONS, defaultOption);

	http_listener nextStepListener(utility::conversions::to_utf8string("http://localhost:5000/api/getnextmove/"));
	nextStepListener.support(methods::POST, getNextStep);
	nextStepListener.support(methods::OPTIONS, defaultOption);

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
