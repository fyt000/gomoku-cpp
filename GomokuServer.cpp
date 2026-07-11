#include <httplib.h>

#include <array>
#include <exception>
#include <glaze/glaze.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "Gomoku.h"
#include "RowEvaluator.h"

namespace gomoku::server {

using BoardCells = std::array<std::array<Piece, kBoardSize>, kBoardSize>;

struct BoardRequest {
  std::vector<int> board;
};

struct WinnerResponse {
  int winner;
};

struct MoveResponse {
  int x;
  int y;
};

void setCorsHeaders(httplib::Response& response)
{
  response.set_header("Access-Control-Allow-Origin", "*");
  response.set_header("Access-Control-Allow-Methods", "POST, OPTIONS");
  response.set_header("Access-Control-Allow-Headers", "Content-Type");
}

template <typename Payload>
void sendJson(httplib::Response& response, const Payload& payload)
{
  std::string body;
  if (glz::write_json(payload, body)) {
    response.status = 500;
    return;
  }
  setCorsHeaders(response);
  response.set_content(body, "application/json");
}

bool parseBoard(const httplib::Request& request, BoardCells& cells)
{
  BoardRequest payload;
  if (glz::read<glz::opts{.error_on_missing_keys = true}>(payload, request.body) ||
      payload.board.size() != kBoardSize * kBoardSize) {
    return false;
  }

  for (int row = 0; row < kBoardSize; ++row) {
    for (int column = 0; column < kBoardSize; ++column) {
      const int value = payload.board[row * kBoardSize + column];
      if (value < Piece::EMPTY || value > Piece::WHITE) {
        return false;
      }
      cells[row][column] = static_cast<Piece>(value);
    }
  }
  return true;
}

template <typename Handler>
void handleBoardRequest(const httplib::Request& request, httplib::Response& response,
                        const std::vector<int>& patternEvals1,
                        const std::vector<int>& patternEvals2, Handler&& handler)
{
  BoardCells cells{};
  if (!parseBoard(request, cells)) {
    response.status = 204;
    setCorsHeaders(response);
    return;
  }

  Piece board[kBoardSize][kBoardSize];
  for (int row = 0; row < kBoardSize; ++row) {
    for (int column = 0; column < kBoardSize; ++column) {
      board[row][column] = cells[row][column];
    }
  }

  Gomoku game(patternEvals1, patternEvals2);
  game.setBoard(board);
  sendJson(response, handler(game));
}

int run(int argc, char** argv)
{
  if (argc != 2) {
    std::cerr << "Usage: gomoku-server <pattern-file>\n";
    return 1;
  }

  std::vector<int> patternEvals1;
  std::vector<int> patternEvals2;
  RowEvaluator rowEvaluator;
  rowEvaluator.setPatterns(argv[1], patternEvals1, patternEvals2);

  httplib::Server server;
  const auto optionsHandler = [](const httplib::Request&, httplib::Response& response) {
    response.status = 204;
    setCorsHeaders(response);
  };
  server.Options("/api/iswinner/", optionsHandler);
  server.Options("/api/getnextmove/", optionsHandler);

  server.Post("/api/iswinner/", [&patternEvals1, &patternEvals2](const httplib::Request& request,
                                                                 httplib::Response& response) {
    handleBoardRequest(request, response, patternEvals1, patternEvals2,
                       [](Gomoku& game) { return WinnerResponse{.winner = game.checkWinner()}; });
  });
  server.Post("/api/getnextmove/", [&patternEvals1, &patternEvals2](const httplib::Request& request,
                                                                    httplib::Response& response) {
    handleBoardRequest(request, response, patternEvals1, patternEvals2, [](Gomoku& game) {
      const auto [x, y] = game.placePiece();
      return MoveResponse{.x = x, .y = y};
    });
  });

  std::thread serverThread([&server] { server.listen("0.0.0.0", 5000); });
  std::cout << "Server listening on http://0.0.0.0:5000. Press Enter to exit.\n";
  std::string line;
  std::getline(std::cin, line);
  server.stop();
  serverThread.join();
  return 0;
}

}  // namespace gomoku::server

int main(int argc, char** argv)
{
  try {
    return gomoku::server::run(argc, argv);
  } catch (const std::exception& error) {
    std::cerr << "Error: " << error.what() << '\n';
    return 1;
  }
}
