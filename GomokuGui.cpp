#include <raylib.h>

#ifdef BLACK
#undef BLACK
#endif
#ifdef WHITE
#undef WHITE
#endif

#include <chrono>
#include <cmath>
#include <exception>
#include <future>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "GameSession.h"
#include "RowEvaluator.h"

namespace gomoku::gui {

namespace {

constexpr int kWindowWidth = 1040;
constexpr int kWindowHeight = 790;
constexpr float kBoardLeft = 50.0F;
constexpr float kBoardTop = 75.0F;
constexpr float kCellSize = 45.0F;
constexpr float kPanelLeft = 730.0F;
constexpr float kPanelTop = 51.0F;
constexpr float kPanelWidth = 290.0F;
constexpr float kPanelHeight = 678.0F;
constexpr float kButtonLeft = 760.0F;
constexpr float kButtonWidth = 230.0F;
constexpr float kFirstButtonTop = 620.0F;
constexpr float kSecondButtonTop = 675.0F;
constexpr float kButtonHeight = 46.0F;
constexpr Color kBoardColor{222, 179, 104, 255};
constexpr Color kPanelColor{35, 43, 54, 255};
constexpr Color kAccentColor{85, 174, 128, 255};

struct LastComputerMove {
  int row = -1;
  int column = -1;
};

const char* pieceName(Piece piece)
{
  switch (piece) {
    case Piece::BLACK:
      return "Black";
    case Piece::WHITE:
      return "White";
    case Piece::EMPTY:
      return "Nobody";
  }
  return "Nobody";
}

std::string coordinateName(int row, int column)
{
  // Common Gomoku notation uses A-O and 1-15, with the central point as H8.
  std::string coordinate(1, static_cast<char>('A' + column));
  coordinate += std::to_string(kBoardSize - row);
  return coordinate;
}

void logMove(const char* player, Piece piece, int row, int column)
{
  std::cout << player << " (" << pieceName(piece) << ") played "
            << coordinateName(row, column) << " (row " << row << ", column " << column << ")\n"
            << std::flush;
}

bool buttonClicked(Rectangle bounds, bool enabled = true)
{
  return enabled && CheckCollisionPointRec(GetMousePosition(), bounds) &&
         IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void drawButton(Rectangle bounds, const char* label, bool enabled = true)
{
  const bool hovered = enabled && CheckCollisionPointRec(GetMousePosition(), bounds);
  DrawRectangleRec(bounds, enabled ? (hovered ? kAccentColor : Color{55, 125, 92, 255})
                                   : Color{86, 92, 100, 255});
  DrawRectangleLinesEx(bounds, 1.0F, Color{220, 225, 230, 255});

  const int textWidth = MeasureText(label, 20);
  DrawText(label, static_cast<int>(bounds.x + (bounds.width - textWidth) / 2.0F),
           static_cast<int>(bounds.y + (bounds.height - 20.0F) / 2.0F), 20, RAYWHITE);
}

void drawThinking(float x, float y)
{
  constexpr const char* kLabel = "Thinking";
  constexpr int kFontSize = 22;
  const int textWidth = MeasureText(kLabel, kFontSize);
  const float progress = std::fmod(static_cast<float>(GetTime()), 1.4F) / 1.4F;
  const float glowCenter = -textWidth * 0.25F + progress * textWidth * 1.5F;
  const int glowWidth = textWidth / 3;

  DrawText(kLabel, static_cast<int>(x), static_cast<int>(y), kFontSize, Color{145, 151, 160, 255});
  BeginScissorMode(static_cast<int>(x + glowCenter - glowWidth / 2.0F), static_cast<int>(y - 2.0F),
                   glowWidth, kFontSize + 4);
  DrawText(kLabel, static_cast<int>(x), static_cast<int>(y), kFontSize, RAYWHITE);
  EndScissorMode();
}

void drawWinningPieces(const GameSession& game)
{
  const Piece winner = static_cast<Piece>(game.checkWinner());
  if (winner == Piece::EMPTY) {
    return;
  }

  constexpr std::pair<int, int> kDirections[] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};
  constexpr Color kWinningRing{42, 155, 255, 255};
  for (int row = 0; row < kBoardSize; ++row) {
    for (int column = 0; column < kBoardSize; ++column) {
      if (game.getPiece(row, column) != winner) {
        continue;
      }

      for (const auto [deltaRow, deltaColumn] : kDirections) {
        if (game.getPiece(row - deltaRow, column - deltaColumn) == winner) {
          continue;
        }

        int lineLength = 0;
        while (game.getPiece(row + lineLength * deltaRow,
                             column + lineLength * deltaColumn) == winner) {
          ++lineLength;
        }
        if (lineLength < 5) {
          continue;
        }

        for (int index = 0; index < 5; ++index) {
          const Vector2 center{kBoardLeft + (column + index * deltaColumn) * kCellSize,
                               kBoardTop + (row + index * deltaRow) * kCellSize};
          DrawCircleLinesV(center, kCellSize * 0.44F, kWinningRing);
          DrawCircleLinesV(center, kCellSize * 0.46F, kWinningRing);
        }
        return;
      }
    }
  }
}

void drawBoard(const GameSession& game, const LastComputerMove& lastComputerMove)
{
  DrawRectangle(static_cast<int>(kBoardLeft - 24.0F), static_cast<int>(kBoardTop - 24.0F),
                static_cast<int>(kCellSize * (kBoardSize - 1) + 48.0F),
                static_cast<int>(kCellSize * (kBoardSize - 1) + 48.0F), kBoardColor);

  for (int index = 0; index < kBoardSize; ++index) {
    const float offset = kCellSize * index;
    const std::string columnLabel(1, static_cast<char>('A' + index));
    const std::string rowLabel = std::to_string(kBoardSize - index);
    const int columnLabelWidth = MeasureText(columnLabel.c_str(), 14);
    const int rowLabelWidth = MeasureText(rowLabel.c_str(), 14);
    DrawText(columnLabel.c_str(),
             static_cast<int>(kBoardLeft + offset - columnLabelWidth / 2.0F),
             static_cast<int>(kBoardTop - 19.0F), 14, Color{76, 56, 34, 255});
    DrawText(rowLabel.c_str(), static_cast<int>(kBoardLeft - 9.0F - rowLabelWidth),
             static_cast<int>(kBoardTop + offset - 7.0F), 14, Color{76, 56, 34, 255});

    DrawLineEx({kBoardLeft, kBoardTop + offset},
               {kBoardLeft + kCellSize * (kBoardSize - 1), kBoardTop + offset}, 1.5F,
               Color{76, 56, 34, 255});
    DrawLineEx({kBoardLeft + offset, kBoardTop},
               {kBoardLeft + offset, kBoardTop + kCellSize * (kBoardSize - 1)}, 1.5F,
               Color{76, 56, 34, 255});
  }

  for (const int coordinate : {3, 7, 11}) {
    DrawCircleV({kBoardLeft + coordinate * kCellSize, kBoardTop + coordinate * kCellSize}, 4.0F,
                Color{76, 56, 34, 255});
  }

  for (int row = 0; row < kBoardSize; ++row) {
    for (int column = 0; column < kBoardSize; ++column) {
      const Piece piece = game.getPiece(row, column);
      if (piece == Piece::EMPTY) {
        continue;
      }

      const Vector2 center{kBoardLeft + column * kCellSize, kBoardTop + row * kCellSize};
      const Color fill = piece == Piece::BLACK ? Color{28, 31, 35, 255} : Color{244, 244, 240, 255};
      DrawCircleV(center, kCellSize * 0.39F, fill);
      DrawCircleLinesV(center, kCellSize * 0.39F, Color{45, 45, 45, 255});
    }
  }

  if (lastComputerMove.row >= 0) {
    const Vector2 center{kBoardLeft + lastComputerMove.column * kCellSize,
                         kBoardTop + lastComputerMove.row * kCellSize};
    constexpr Color kMoveMarkerColor{42, 155, 255, 255};
    DrawCircleLinesV(center, kCellSize * 0.44F, kMoveMarkerColor);
    DrawCircleLinesV(center, kCellSize * 0.46F, kMoveMarkerColor);
  }

  drawWinningPieces(game);
}

void setStatusForGame(const GameSession& game, Piece human, std::string& status)
{
  const Piece winner = static_cast<Piece>(game.checkWinner());
  if (winner != Piece::EMPTY) {
    status = winner == human ? "You win!" : "Computer wins.";
  } else if (game.isBoardFull()) {
    status = "Draw.";
  } else if (game.currentTurn() == human) {
    status = "Your turn: " + std::string(pieceName(human));
  } else {
    status = "Computer turn.";
  }
}

void startComputerMove(const GameSession& game, Piece human, std::string& status,
                       std::future<std::pair<int, int>>& pendingMove, bool& computerThinking)
{
  if (game.checkWinner() || game.isBoardFull() || game.currentTurn() == human) {
    setStatusForGame(game, human, status);
    return;
  }

  GameSession snapshot = game;
  pendingMove = std::async(std::launch::async,
                           [snapshot]() mutable { return snapshot.placeComputerMove(); });
  computerThinking = true;
  status = "Thinking";
}

void collectComputerMove(GameSession& game, Piece human, std::string& status,
                         std::future<std::pair<int, int>>& pendingMove, bool& computerThinking,
                         LastComputerMove& lastComputerMove)
{
  if (!computerThinking ||
      pendingMove.wait_for(std::chrono::milliseconds{0}) != std::future_status::ready) {
    return;
  }

  const auto [row, column] = pendingMove.get();
  computerThinking = false;
  if (row >= 0 && column >= 0) {
    const Piece computer = game.currentTurn();
    if (game.placePiece(row, column)) {
      logMove("Computer", computer, row, column);
      lastComputerMove = {.row = row, .column = column};
    }
  }
  setStatusForGame(game, human, status);
}

void startGame(GameSession& game, Piece human, std::string& status,
               std::future<std::pair<int, int>>& pendingMove, bool& computerThinking,
               LastComputerMove& lastComputerMove)
{
  game.reset();
  lastComputerMove = {};
  std::cout << "New game: you are " << pieceName(human)
            << ". Move notation uses A-O and 1-15; zero-based row and column follow.\n"
            << std::flush;
  setStatusForGame(game, human, status);
  startComputerMove(game, human, status, pendingMove, computerThinking);
}

void returnToSideSelection(GameSession& game, bool& gameStarted, std::string& status,
                           LastComputerMove& lastComputerMove)
{
  game.reset();
  gameStarted = false;
  lastComputerMove = {};
  status = "Choose a side.";
  std::cout << "New game: choose Black or White.\n" << std::flush;
}

bool boardClickCoordinates(int& row, int& column)
{
  if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    return false;
  }

  const Vector2 mouse = GetMousePosition();
  column = static_cast<int>((mouse.x - kBoardLeft) / kCellSize + 0.5F);
  row = static_cast<int>((mouse.y - kBoardTop) / kCellSize + 0.5F);
  if (row < 0 || row >= kBoardSize || column < 0 || column >= kBoardSize) {
    return false;
  }

  const Vector2 intersection{kBoardLeft + column * kCellSize, kBoardTop + row * kCellSize};
  const float deltaX = mouse.x - intersection.x;
  const float deltaY = mouse.y - intersection.y;
  if (std::sqrt(deltaX * deltaX + deltaY * deltaY) > kCellSize * 0.43F) {
    return false;
  }

  return true;
}

bool boardClickToMove(GameSession& game, Piece human, bool gameStarted, std::string& status,
                      std::future<std::pair<int, int>>& pendingMove, bool& computerThinking)
{
  if (!gameStarted || game.currentTurn() != human || game.checkWinner() || game.isBoardFull()) {
    return false;
  }

  int row = 0;
  int column = 0;
  if (!boardClickCoordinates(row, column)) {
    return false;
  }

  if (!game.placePiece(row, column)) {
    status = "Occupied.";
    return false;
  }

  logMove("You", human, row, column);
  startComputerMove(game, human, status, pendingMove, computerThinking);
  return true;
}

void undoRound(GameSession& game, Piece human, std::string& status,
               LastComputerMove& lastComputerMove)
{
  // Keep the computer's opening black stone when the player chose white.
  const int minimumMoves = human == Piece::WHITE ? 1 : 0;
  if (game.moveCount() <= minimumMoves) {
    status = "There is no completed turn to retract.";
    return;
  }

  do {
    (void)game.undo();
  } while (game.moveCount() > minimumMoves && game.currentTurn() != human);
  lastComputerMove = {};
  setStatusForGame(game, human, status);
}

bool canRetract(const GameSession& game, Piece human)
{
  const int minimumMoves = human == Piece::WHITE ? 1 : 0;
  return game.moveCount() > minimumMoves;
}

}  // namespace

int run(int argc, char** argv)
{
  if (argc != 2) {
    TraceLog(LOG_ERROR, "Usage: gomoku-gui <pattern-file>");
    return 1;
  }

  std::vector<int> patternEvals1;
  std::vector<int> patternEvals2;
  RowEvaluator rowEvaluator;
  rowEvaluator.setPatterns(argv[1], patternEvals1, patternEvals2);
  GameSession game(patternEvals1, patternEvals2);

  Piece human = Piece::BLACK;
  bool gameStarted = false;
  bool computerThinking = false;
  std::future<std::pair<int, int>> pendingComputerMove;
  LastComputerMove lastComputerMove;
  std::string status = "Choose a side.";

  InitWindow(kWindowWidth, kWindowHeight, "Gomoku");
  SetTargetFPS(60);
  while (!WindowShouldClose()) {
    const Rectangle firstButton{kButtonLeft, kFirstButtonTop, kButtonWidth, kButtonHeight};
    const Rectangle secondButton{kButtonLeft, kSecondButtonTop, kButtonWidth, kButtonHeight};
    collectComputerMove(game, human, status, pendingComputerMove, computerThinking,
                        lastComputerMove);
    if (!gameStarted) {
      if (buttonClicked(firstButton)) {
        human = Piece::BLACK;
        gameStarted = true;
        startGame(game, human, status, pendingComputerMove, computerThinking, lastComputerMove);
      } else if (buttonClicked(secondButton)) {
        human = Piece::WHITE;
        gameStarted = true;
        startGame(game, human, status, pendingComputerMove, computerThinking, lastComputerMove);
      } else {
        int row = 0;
        int column = 0;
        if (boardClickCoordinates(row, column)) {
          human = Piece::BLACK;
          gameStarted = true;
          startGame(game, human, status, pendingComputerMove, computerThinking,
                    lastComputerMove);
        }
      }
    } else {
      if (buttonClicked(firstButton, !computerThinking && canRetract(game, human))) {
        undoRound(game, human, status, lastComputerMove);
      }
      if (buttonClicked(secondButton, !computerThinking)) {
        returnToSideSelection(game, gameStarted, status, lastComputerMove);
      }
    }
    (void)boardClickToMove(game, human, gameStarted, status, pendingComputerMove,
                           computerThinking);

    BeginDrawing();
    ClearBackground(Color{240, 242, 245, 255});
    drawBoard(game, lastComputerMove);

    DrawRectangle(static_cast<int>(kPanelLeft), static_cast<int>(kPanelTop),
                  static_cast<int>(kPanelWidth), static_cast<int>(kPanelHeight), kPanelColor);
    if (computerThinking) {
      drawThinking(760.0F, 84.0F);
    } else {
      DrawText(status.c_str(), 760, 84, 22, RAYWHITE);
    }
    const Piece winner = static_cast<Piece>(game.checkWinner());
    const char* hint = !gameStarted ? "Black moves first."
                       : winner != Piece::EMPTY || game.isBoardFull()
                           ? "Start a new game to play again."
                       : computerThinking ? "Your move is on the board."
                                         : "Click board to play.";
    DrawText(hint, 760, 120, 17, Color{185, 194, 202, 255});
    if (gameStarted) {
      drawButton(firstButton, "Retract turn", !computerThinking && canRetract(game, human));
      drawButton(secondButton, "New game", !computerThinking);
    } else {
      drawButton(firstButton, "Play Black (first)");
      drawButton(secondButton, "Play White (second)");
    }
    EndDrawing();
  }

  CloseWindow();
  return 0;
}

}  // namespace gomoku::gui

int main(int argc, char** argv)
{
  try {
    return gomoku::gui::run(argc, argv);
  } catch (const std::exception& error) {
    TraceLog(LOG_ERROR, "Error: %s", error.what());
    return 1;
  }
}
