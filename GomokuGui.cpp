#include <raylib.h>

#ifdef BLACK
#undef BLACK
#endif
#ifdef WHITE
#undef WHITE
#endif

#include <cmath>
#include <exception>
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

void drawBoard(const GameSession& game)
{
  DrawRectangle(static_cast<int>(kBoardLeft - 24.0F), static_cast<int>(kBoardTop - 24.0F),
                static_cast<int>(kCellSize * (kBoardSize - 1) + 48.0F),
                static_cast<int>(kCellSize * (kBoardSize - 1) + 48.0F), kBoardColor);

  for (int index = 0; index < kBoardSize; ++index) {
    const float offset = kCellSize * index;
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

void makeComputerMove(GameSession& game, Piece human, std::string& status)
{
  if (game.checkWinner() || game.isBoardFull() || game.currentTurn() == human) {
    setStatusForGame(game, human, status);
    return;
  }

  (void)game.placeComputerMove();
  setStatusForGame(game, human, status);
}

void startGame(GameSession& game, Piece human, std::string& status)
{
  game.reset();
  setStatusForGame(game, human, status);
  makeComputerMove(game, human, status);
}

bool boardClickToMove(GameSession& game, Piece human, bool gameStarted, std::string& status)
{
  if (!gameStarted || game.currentTurn() != human || game.checkWinner() || game.isBoardFull() ||
      !IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    return false;
  }

  const Vector2 mouse = GetMousePosition();
  const int column = static_cast<int>((mouse.x - kBoardLeft) / kCellSize + 0.5F);
  const int row = static_cast<int>((mouse.y - kBoardTop) / kCellSize + 0.5F);
  if (row < 0 || row >= kBoardSize || column < 0 || column >= kBoardSize) {
    return false;
  }

  const Vector2 intersection{kBoardLeft + column * kCellSize, kBoardTop + row * kCellSize};
  const float deltaX = mouse.x - intersection.x;
  const float deltaY = mouse.y - intersection.y;
  if (std::sqrt(deltaX * deltaX + deltaY * deltaY) > kCellSize * 0.43F) {
    return false;
  }

  if (!game.placePiece(row, column)) {
    status = "Occupied.";
    return false;
  }

  makeComputerMove(game, human, status);
  return true;
}

void undoRound(GameSession& game, Piece human, std::string& status)
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
  std::string status = "Choose a side.";

  InitWindow(kWindowWidth, kWindowHeight, "Gomoku");
  SetTargetFPS(60);
  while (!WindowShouldClose()) {
    const Rectangle firstButton{kButtonLeft, kFirstButtonTop, kButtonWidth, kButtonHeight};
    const Rectangle secondButton{kButtonLeft, kSecondButtonTop, kButtonWidth, kButtonHeight};
    if (!gameStarted) {
      if (buttonClicked(firstButton)) {
        human = Piece::BLACK;
        gameStarted = true;
        startGame(game, human, status);
      }
      if (buttonClicked(secondButton)) {
        human = Piece::WHITE;
        gameStarted = true;
        startGame(game, human, status);
      }
    } else {
      if (buttonClicked(firstButton, canRetract(game, human))) {
        undoRound(game, human, status);
      }
      if (buttonClicked(secondButton)) {
        startGame(game, human, status);
      }
    }
    (void)boardClickToMove(game, human, gameStarted, status);

    BeginDrawing();
    ClearBackground(Color{240, 242, 245, 255});
    drawBoard(game);

    DrawRectangle(static_cast<int>(kPanelLeft), static_cast<int>(kPanelTop),
                  static_cast<int>(kPanelWidth), static_cast<int>(kPanelHeight), kPanelColor);
    DrawText(status.c_str(), 760, 84, 22, RAYWHITE);
    DrawText(gameStarted ? "Click board to play." : "Black moves first.", 760, 120, 17,
             Color{185, 194, 202, 255});
    if (gameStarted) {
      drawButton(firstButton, "Retract turn", canRetract(game, human));
      drawButton(secondButton, "New game");
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
