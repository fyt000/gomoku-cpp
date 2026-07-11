#include <cmath>
#include <exception>
#include <glaze/glaze.hpp>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "GameSession.h"
#include "RowEvaluator.h"

namespace gomoku::mcp {

namespace {

using Json = glz::generic;

const Json* field(const Json& object, std::string_view name)
{
  const auto* values = object.get_if<Json::object_t>();
  if (!values) {
    return nullptr;
  }
  const auto position = values->find(name);
  return position == values->end() ? nullptr : &position->second;
}

bool readInteger(const Json* value, int& output)
{
  if (!value || !value->is_number()) {
    return false;
  }
  const double number = value->get_number();
  if (std::floor(number) != number ||
      number < static_cast<double>(std::numeric_limits<int>::min()) ||
      number > static_cast<double>(std::numeric_limits<int>::max())) {
    return false;
  }
  output = static_cast<int>(number);
  return true;
}

const char* pieceName(Piece piece)
{
  switch (piece) {
    case Piece::BLACK:
      return "black";
    case Piece::WHITE:
      return "white";
    case Piece::EMPTY:
      return "none";
  }
  return "none";
}

Json gameState(const GameSession& game)
{
  Json state;
  Json::array_t board;
  board.reserve(kBoardSize * kBoardSize);
  for (int row = 0; row < kBoardSize; ++row) {
    for (int column = 0; column < kBoardSize; ++column) {
      board.emplace_back(static_cast<int>(game.getPiece(row, column)));
    }
  }

  state["board"] = board;
  state["currentTurn"] = pieceName(game.currentTurn());
  state["moveCount"] = game.moveCount();
  const Piece winner = game.checkWinner();
  if (winner == Piece::EMPTY) {
    state["winner"] = nullptr;
  } else {
    state["winner"] = pieceName(winner);
  }
  state["isDraw"] = winner == Piece::EMPTY && game.isBoardFull();
  return state;
}

Json textResult(const Json& value, bool isError = false)
{
  std::string text;
  if (glz::write_json(value, text)) {
    text = R"({"error":"Unable to serialize game state"})";
    isError = true;
  }

  Json contentItem;
  contentItem["type"] = "text";
  contentItem["text"] = text;
  Json::array_t content;
  content.push_back(std::move(contentItem));

  Json result;
  result["content"] = content;
  result["structuredContent"] = value;
  if (isError) {
    result["isError"] = true;
  }
  return result;
}

Json textError(std::string_view message)
{
  Json error;
  error["error"] = message;
  return textResult(error, true);
}

void sendResponse(const Json& id, Json result)
{
  Json response;
  response["jsonrpc"] = "2.0";
  response["id"] = id;
  response["result"] = std::move(result);

  std::string serialized;
  if (!glz::write_json(response, serialized)) {
    std::cout << serialized << '\n' << std::flush;
  }
}

void sendError(const Json& id, int code, std::string_view message)
{
  Json error;
  error["code"] = code;
  error["message"] = message;

  Json response;
  response["jsonrpc"] = "2.0";
  response["id"] = id;
  response["error"] = std::move(error);

  std::string serialized;
  if (!glz::write_json(response, serialized)) {
    std::cout << serialized << '\n' << std::flush;
  }
}

Json toolsList()
{
  constexpr std::string_view kTools = R"json(
{
  "tools": [
    {
      "name": "new_game",
      "description": "Start a fresh Gomoku game. Black always moves first.",
      "inputSchema": {"type": "object", "additionalProperties": false}
    },
    {
      "name": "game_state",
      "description": "Read the current board, side to move, winner, and move count.",
      "inputSchema": {"type": "object", "additionalProperties": false}
    },
    {
      "name": "play_move",
      "description": "Place a stone for the current side to move. Coordinates are zero-based: x is the row and y is the column.",
      "inputSchema": {
        "type": "object",
        "properties": {"x": {"type": "integer", "minimum": 0, "maximum": 14}, "y": {"type": "integer", "minimum": 0, "maximum": 14}},
        "required": ["x", "y"],
        "additionalProperties": false
      }
    },
    {
      "name": "computer_move",
      "description": "Ask the built-in Gomoku engine to make the current side's move.",
      "inputSchema": {"type": "object", "additionalProperties": false}
    }
  ]
}
)json";

  Json tools;
  (void)glz::read<glz::opts{}>(tools, kTools);
  return tools;
}

Json runTool(GameSession& game, std::string_view name, const Json* arguments)
{
  if (name == "new_game") {
    game.reset();
    return textResult(gameState(game));
  }
  if (name == "game_state") {
    return textResult(gameState(game));
  }
  if (name == "play_move") {
    int x = -1;
    int y = -1;
    if (!arguments || !readInteger(field(*arguments, "x"), x) ||
        !readInteger(field(*arguments, "y"), y)) {
      return textError("play_move requires integer x and y coordinates.");
    }
    if (game.checkWinner() != Piece::EMPTY || game.isBoardFull()) {
      return textError("The game is already finished. Call new_game to start again.");
    }
    if (!game.placePiece(x, y)) {
      return textError("The move is outside the board or the intersection is occupied.");
    }
    return textResult(gameState(game));
  }
  if (name == "computer_move") {
    if (game.checkWinner() != Piece::EMPTY || game.isBoardFull()) {
      return textError("The game is already finished. Call new_game to start again.");
    }
    const auto [x, y] = game.placeComputerMove();
    if (x < 0 || y < 0) {
      return textError("The engine could not find a legal move.");
    }
    Json state = gameState(game);
    Json move;
    move["x"] = x;
    move["y"] = y;
    state["move"] = std::move(move);
    return textResult(state);
  }
  return textError("Unknown tool name.");
}

void handleRequest(GameSession& game, const Json& request)
{
  const Json* id = field(request, "id");
  const Json* method = field(request, "method");
  const bool hasId = id != nullptr;

  if (!method || !method->is_string()) {
    if (hasId) {
      sendError(*id, -32600, "Invalid JSON-RPC request.");
    }
    return;
  }

  const std::string& methodName = method->get_string();
  if (methodName == "notifications/initialized") {
    return;
  }
  if (methodName == "initialize") {
    if (hasId) {
      Json result;
      result["protocolVersion"] = "2025-06-18";
      result["capabilities"] = Json::object_t{};
      result["capabilities"]["tools"] = Json::object_t{};
      result["serverInfo"]["name"] = "gomoku-cpp";
      result["serverInfo"]["version"] = "1.0.0";
      sendResponse(*id, std::move(result));
    }
    return;
  }
  if (methodName == "tools/list") {
    if (hasId) {
      sendResponse(*id, toolsList());
    }
    return;
  }
  if (methodName == "tools/call") {
    const Json* params = field(request, "params");
    const Json* toolName = params ? field(*params, "name") : nullptr;
    if (!toolName || !toolName->is_string()) {
      if (hasId) {
        sendError(*id, -32602, "tools/call requires a tool name.");
      }
      return;
    }
    if (hasId) {
      sendResponse(*id, runTool(game, toolName->get_string(), field(*params, "arguments")));
    }
    return;
  }

  if (hasId) {
    sendError(*id, -32601, "Method not found.");
  }
}

}  // namespace

int run(int argc, char** argv)
{
  if (argc != 2) {
    std::cerr << "Usage: gomoku-mcp <pattern-file>\n";
    return 1;
  }

  RowEvaluator rowEvaluator;
  const EvaluationTablesPtr evaluationTables = rowEvaluator.loadPatterns(argv[1]);
  GameSession game(evaluationTables);

  std::string line;
  while (std::getline(std::cin, line)) {
    Json request;
    if (glz::read<glz::opts{}>(request, line)) {
      Json nullId;
      sendError(nullId, -32700, "Parse error.");
      continue;
    }
    handleRequest(game, request);
  }
  return 0;
}

}  // namespace gomoku::mcp

int main(int argc, char** argv)
{
  try {
    return gomoku::mcp::run(argc, argv);
  } catch (const std::exception& error) {
    std::cerr << "Error: " << error.what() << '\n';
    return 1;
  }
}
