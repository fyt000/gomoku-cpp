#include <httplib.h>

#include <array>
#include <exception>
#include <glaze/glaze.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "CompiledEvaluationTables.h"
#include "Gomoku.h"
#include "LicenseNotices.h"
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
  setCorsHeaders(response);
  std::string body;
  if (glz::write_json(payload, body)) {
    response.status = 500;
    response.set_content("Failed to serialize JSON response\n", "text/plain");
    return;
  }
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
      if (value < static_cast<int>(Piece::EMPTY) || value > static_cast<int>(Piece::WHITE)) {
        return false;
      }
      cells[row][column] = static_cast<Piece>(value);
    }
  }
  return true;
}

template <typename Handler>
void handleBoardRequest(const httplib::Request& request, httplib::Response& response,
                        const EvaluationTablesPtr& evaluationTables, Handler&& handler)
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

  Gomoku game(evaluationTables);
  game.setBoard(board);
  sendJson(response, handler(game));
}

int run(int argc, char** argv)
{
  if (printLicenseNoticesIfRequested(argc, argv)) {
    return 0;
  }

  if (argc > 2) {
    std::cerr << "Usage: gomoku-server [pattern-file | --licenses]\n";
    return 1;
  }

  const EvaluationTablesPtr evaluationTables =
      argc == 2 ? RowEvaluator{}.loadPatterns(argv[1]) : compiledEvaluationTables();

  httplib::Server server;
  const auto optionsHandler = [](const httplib::Request&, httplib::Response& response) {
    response.status = 204;
    setCorsHeaders(response);
  };
  server.Options("/api/iswinner/", optionsHandler);
  server.Options("/api/getnextmove/", optionsHandler);

  server.Post("/api/iswinner/",
              [evaluationTables](const httplib::Request& request, httplib::Response& response) {
                handleBoardRequest(request, response, evaluationTables, [](Gomoku& game) {
                  return WinnerResponse{.winner = static_cast<int>(game.checkWinner())};
                });
              });
  server.Post("/api/getnextmove/",
              [evaluationTables](const httplib::Request& request, httplib::Response& response) {
                handleBoardRequest(request, response, evaluationTables, [](Gomoku& game) {
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
