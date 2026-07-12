#include <raylib.h>

#ifdef BLACK
#undef BLACK
#endif
#ifdef WHITE
#undef WHITE
#endif

#include <chrono>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <exception>
#include <future>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "CompiledEvaluationTables.h"
#include "GameSession.h"
#include "LicenseNotices.h"
#include "RowEvaluator.h"
#include "SearchEffort.h"

namespace gomoku::gui {

namespace {

constexpr float kCellSize = 45.0F;
constexpr float kBoardPadding = 24.0F;
constexpr float kBoardCanvasSize = kCellSize * (kBoardSize - 1) + 2.0F * kBoardPadding;
constexpr float kBoardLeft = kBoardPadding;
constexpr float kBoardTop = kBoardPadding;
constexpr float kPanelWidth = 290.0F;
constexpr float kPanelHeight = kBoardCanvasSize;
constexpr float kPanelLeft = kBoardCanvasSize;
constexpr float kPanelTop = 0.0F;
constexpr int kWindowWidth = static_cast<int>(kBoardCanvasSize + kPanelWidth);
constexpr int kWindowHeight = static_cast<int>(kBoardCanvasSize);
constexpr float kPanelContentLeft = kPanelLeft + 30.0F;
constexpr float kButtonLeft = kPanelContentLeft;
constexpr float kButtonWidth = 230.0F;
constexpr float kFirstButtonTop = 569.0F;
constexpr float kSecondButtonTop = 624.0F;
constexpr float kButtonHeight = 46.0F;
constexpr float kEffortTop = 133.0F;
constexpr float kSeedTop = 221.0F;
constexpr float kSeedFieldTop = 246.0F;
constexpr float kSeedFieldHeight = 36.0F;
constexpr float kSeedButtonTop = 290.0F;
constexpr float kSeedButtonHeight = 36.0F;
constexpr float kSeedButtonGap = 7.0F;
constexpr float kEffortArrowWidth = 46.0F;
constexpr float kEffortValueWidth = kButtonWidth - 2.0F * kEffortArrowWidth;
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
  std::cout << player << " (" << pieceName(piece) << ") played " << coordinateName(row, column)
            << " (row " << row << ", column " << column << ")\n"
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

struct EffortSelectorBounds {
  Rectangle previous;
  Rectangle value;
  Rectangle next;
};

struct SeedEditorBounds {
  Rectangle field;
  Rectangle apply;
  Rectangle random;
  Rectangle copy;
};

SeedEditorBounds seedEditorBounds()
{
  constexpr float applyWidth = 64.0F;
  constexpr float randomWidth = 91.0F;
  constexpr float copyWidth =
      kButtonWidth - applyWidth - randomWidth - 2.0F * kSeedButtonGap;
  return {
      .field = {kButtonLeft, kSeedFieldTop, kButtonWidth, kSeedFieldHeight},
      .apply = {kButtonLeft, kSeedButtonTop, applyWidth, kSeedButtonHeight},
      .random = {kButtonLeft + applyWidth + kSeedButtonGap, kSeedButtonTop, randomWidth,
                 kSeedButtonHeight},
      .copy = {kButtonLeft + applyWidth + kSeedButtonGap + randomWidth + kSeedButtonGap,
               kSeedButtonTop, copyWidth, kSeedButtonHeight},
  };
}

EffortSelectorBounds effortSelectorBounds()
{
  return {
      .previous = {kButtonLeft, kEffortTop, kEffortArrowWidth, kButtonHeight},
      .value = {kButtonLeft + kEffortArrowWidth, kEffortTop, kEffortValueWidth, kButtonHeight},
      .next = {kButtonLeft + kEffortArrowWidth + kEffortValueWidth, kEffortTop, kEffortArrowWidth,
               kButtonHeight},
  };
}

void updateEffortSelector(SearchEffort& effort, bool enabled)
{
  const EffortSelectorBounds bounds = effortSelectorBounds();
  int index = static_cast<int>(effort);
  if (buttonClicked(bounds.previous, enabled && index > 0)) {
    effort = static_cast<SearchEffort>(index - 1);
  } else if (buttonClicked(bounds.next,
                           enabled && index + 1 < static_cast<int>(kSearchEffortPresets.size()))) {
    effort = static_cast<SearchEffort>(index + 1);
  }
}

void drawEffortSelector(SearchEffort effort, bool enabled)
{
  const EffortSelectorBounds bounds = effortSelectorBounds();
  const int index = static_cast<int>(effort);
  const SearchEffortPreset& preset = searchEffortPreset(effort);
  const SearchOptions options = searchOptionsForEffort(effort);

  DrawText("Effort", static_cast<int>(kButtonLeft), static_cast<int>(kEffortTop - 28.0F), 18,
           Color{200, 207, 216, 255});
  drawButton(bounds.previous, "<", enabled && index > 0);
  drawButton(bounds.next, ">",
             enabled && index + 1 < static_cast<int>(kSearchEffortPresets.size()));

  DrawRectangleRec(bounds.value, enabled ? Color{47, 56, 68, 255} : Color{67, 73, 81, 255});
  DrawRectangleLinesEx(bounds.value, 1.0F, Color{220, 225, 230, 255});
  const int nameWidth = MeasureText(preset.displayName.data(), 20);
  DrawText(preset.displayName.data(),
           static_cast<int>(bounds.value.x + (bounds.value.width - nameWidth) / 2.0F),
           static_cast<int>(bounds.value.y + 13.0F), 20, RAYWHITE);

  std::string details = "Depth " + std::to_string(options.depth) + "  |  Radius " +
                        std::to_string(options.frontierRadius) + "  |  " +
                        std::to_string(options.threadCount) +
                        (options.threadCount == 1 ? " thread" : " threads");
  DrawText(details.c_str(), static_cast<int>(kButtonLeft), static_cast<int>(kEffortTop + 57.0F), 14,
           Color{170, 179, 190, 255});
}

void drawSeedEditor(const SeedEditorBounds& bounds, const std::string& text, bool focused,
                    bool selected, bool invalid, bool copied, bool editable)
{
  DrawText("Seed", static_cast<int>(kButtonLeft), static_cast<int>(kSeedTop), 18,
           Color{200, 207, 216, 255});

  DrawRectangleRec(bounds.field,
                   editable ? Color{47, 56, 68, 255} : Color{67, 73, 81, 255});
  const Color border = invalid     ? Color{235, 92, 92, 255}
                       : focused   ? Color{85, 174, 128, 255}
                                   : Color{220, 225, 230, 255};
  DrawRectangleLinesEx(bounds.field, focused ? 2.0F : 1.0F, border);
  if (focused && selected && !text.empty()) {
    const int selectionWidth = std::min(MeasureText(text.c_str(), 17),
                                        static_cast<int>(bounds.field.width - 16.0F));
    DrawRectangle(static_cast<int>(bounds.field.x + 8.0F),
                  static_cast<int>(bounds.field.y + 8.0F), selectionWidth, 20, kAccentColor);
  }
  DrawText(text.c_str(), static_cast<int>(bounds.field.x + 8.0F),
           static_cast<int>(bounds.field.y + 9.0F), 17, RAYWHITE);

  drawButton(bounds.apply, "Apply", editable);
  drawButton(bounds.random, "Random", editable);
  drawButton(bounds.copy, copied ? "Done" : "Copy");
}

bool parseSeed(std::string_view text, std::uint64_t& seed)
{
  const char* begin = text.data();
  const char* end = begin + text.size();
  const auto result = std::from_chars(begin, end, seed);
  return result.ec == std::errc{} && result.ptr == end;
}

bool shortcutKeyDown()
{
  return IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL) ||
         IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER);
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
  const Piece winner = game.checkWinner();
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
        while (game.getPiece(row + lineLength * deltaRow, column + lineLength * deltaColumn) ==
               winner) {
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
  DrawRectangle(0, 0, static_cast<int>(kBoardCanvasSize), static_cast<int>(kBoardCanvasSize),
                kBoardColor);

  for (int index = 0; index < kBoardSize; ++index) {
    const float offset = kCellSize * index;
    const std::string columnLabel(1, static_cast<char>('A' + index));
    const std::string rowLabel = std::to_string(kBoardSize - index);
    const int columnLabelWidth = MeasureText(columnLabel.c_str(), 14);
    const int rowLabelWidth = MeasureText(rowLabel.c_str(), 14);
    DrawText(columnLabel.c_str(), static_cast<int>(kBoardLeft + offset - columnLabelWidth / 2.0F),
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
  const Piece winner = game.checkWinner();
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
                       std::future<std::pair<int, int>>& pendingMove, bool& computerThinking,
                       SearchOptions searchOptions)
{
  if (game.checkWinner() != Piece::EMPTY || game.isBoardFull() || game.currentTurn() == human) {
    setStatusForGame(game, human, status);
    return;
  }

  GameSession snapshot = game;
  pendingMove = std::async(std::launch::async, [snapshot, searchOptions]() mutable {
    return snapshot.placeComputerMove(searchOptions);
  });
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
               LastComputerMove& lastComputerMove, SearchOptions searchOptions)
{
  game.reset();
  lastComputerMove = {};
  std::cout << "New game: you are " << pieceName(human)
            << ". Move notation uses A-O and 1-15; zero-based row and column follow.\n"
            << std::flush;
  setStatusForGame(game, human, status);
  startComputerMove(game, human, status, pendingMove, computerThinking, searchOptions);
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
                      std::future<std::pair<int, int>>& pendingMove, bool& computerThinking,
                      SearchOptions searchOptions)
{
  if (!gameStarted || game.currentTurn() != human || game.checkWinner() != Piece::EMPTY ||
      game.isBoardFull()) {
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
  startComputerMove(game, human, status, pendingMove, computerThinking, searchOptions);
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
  if (printLicenseNoticesIfRequested(argc, argv)) {
    return 0;
  }

  std::string_view patternFile;
  std::uint64_t seed = generateRandomSeed();
  for (int argument = 1; argument < argc; ++argument) {
    const std::string_view value = argv[argument];
    if (value == "--seed") {
      if (++argument >= argc || !parseSeed(argv[argument], seed)) {
        TraceLog(LOG_ERROR, "--seed requires an unsigned 64-bit integer");
        return 1;
      }
    } else if (value.starts_with("--seed=")) {
      if (!parseSeed(value.substr(7), seed)) {
        TraceLog(LOG_ERROR, "--seed requires an unsigned 64-bit integer");
        return 1;
      }
    } else if (value.starts_with("--") || !patternFile.empty()) {
      TraceLog(LOG_ERROR, "Usage: gomoku-gui [--seed number] [pattern-file | --licenses]");
      return 1;
    } else {
      patternFile = value;
    }
  }

  const EvaluationTablesPtr evaluationTables =
      !patternFile.empty() ? RowEvaluator{}.loadPatterns(std::string(patternFile))
                           : compiledEvaluationTables();
  GameSession game(evaluationTables);

  Piece human = Piece::BLACK;
  SearchEffort effort = SearchEffort::HIGH;
  bool gameStarted = false;
  bool computerThinking = false;
  std::future<std::pair<int, int>> pendingComputerMove;
  LastComputerMove lastComputerMove;
  std::string status = "Choose a side.";
  std::string seedText = std::to_string(seed);
  bool seedFieldFocused = false;
  bool seedTextSelected = false;
  bool seedTextInvalid = false;
  double seedCopiedUntil = 0.0;
  const auto currentSearchOptions = [&] {
    SearchOptions options = searchOptionsForEffort(effort);
    options.seed = seed;
    return options;
  };

  InitWindow(kWindowWidth, kWindowHeight, "Gomoku");
  SetTargetFPS(60);
  while (!WindowShouldClose()) {
    const Rectangle firstButton{kButtonLeft, kFirstButtonTop, kButtonWidth, kButtonHeight};
    const Rectangle secondButton{kButtonLeft, kSecondButtonTop, kButtonWidth, kButtonHeight};
    const SeedEditorBounds seedBounds = seedEditorBounds();
    collectComputerMove(game, human, status, pendingComputerMove, computerThinking,
                        lastComputerMove);
    updateEffortSelector(effort, !computerThinking);

    const bool mousePressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    if (mousePressed && !computerThinking &&
        CheckCollisionPointRec(GetMousePosition(), seedBounds.field)) {
      seedFieldFocused = true;
      seedTextSelected = true;
      seedTextInvalid = false;
    } else if (mousePressed) {
      seedFieldFocused = false;
      seedTextSelected = false;
    }

    if (computerThinking) {
      seedFieldFocused = false;
      seedTextSelected = false;
    }
    if (seedFieldFocused) {
      if (shortcutKeyDown() && IsKeyPressed(KEY_A)) {
        seedTextSelected = true;
      }
      if (shortcutKeyDown() && IsKeyPressed(KEY_V)) {
        const char* clipboardText = GetClipboardText();
        std::uint64_t pastedSeed = 0;
        if (clipboardText && parseSeed(clipboardText, pastedSeed)) {
          seedText = clipboardText;
          seedTextSelected = false;
          seedTextInvalid = false;
        } else {
          seedTextInvalid = true;
        }
      }

      int character = GetCharPressed();
      while (character > 0) {
        if (character >= '0' && character <= '9') {
          if (seedTextSelected) {
            seedText.clear();
            seedTextSelected = false;
          }
          if (seedText.size() < 20) {
            seedText.push_back(static_cast<char>(character));
            seedTextInvalid = false;
          }
        }
        character = GetCharPressed();
      }
      if (IsKeyPressed(KEY_BACKSPACE)) {
        if (seedTextSelected) {
          seedText.clear();
          seedTextSelected = false;
        } else if (!seedText.empty()) {
          seedText.pop_back();
        }
        seedTextInvalid = false;
      }
      if (IsKeyPressed(KEY_ESCAPE)) {
        seedText = std::to_string(seed);
        seedFieldFocused = false;
        seedTextSelected = false;
        seedTextInvalid = false;
      }
    }

    const bool applySeed = buttonClicked(seedBounds.apply, !computerThinking) ||
                           (seedFieldFocused && IsKeyPressed(KEY_ENTER));
    if (applySeed) {
      std::uint64_t parsedSeed = 0;
      if (parseSeed(seedText, parsedSeed)) {
        seed = parsedSeed;
        seedText = std::to_string(seed);
        seedFieldFocused = false;
        seedTextSelected = false;
        seedTextInvalid = false;
      } else {
        seedFieldFocused = true;
        seedTextInvalid = true;
      }
    }
    if (buttonClicked(seedBounds.random, !computerThinking)) {
      seed = generateRandomSeed();
      seedText = std::to_string(seed);
      seedFieldFocused = false;
      seedTextSelected = false;
      seedTextInvalid = false;
    }
    if (buttonClicked(seedBounds.copy)) {
      const std::string activeSeedText = std::to_string(seed);
      SetClipboardText(activeSeedText.c_str());
      seedCopiedUntil = GetTime() + 1.5;
    }
    if (!gameStarted) {
      if (buttonClicked(firstButton)) {
        human = Piece::BLACK;
        gameStarted = true;
        startGame(game, human, status, pendingComputerMove, computerThinking, lastComputerMove,
                  currentSearchOptions());
      } else if (buttonClicked(secondButton)) {
        human = Piece::WHITE;
        gameStarted = true;
        startGame(game, human, status, pendingComputerMove, computerThinking, lastComputerMove,
                  currentSearchOptions());
      } else {
        int row = 0;
        int column = 0;
        if (boardClickCoordinates(row, column)) {
          human = Piece::BLACK;
          gameStarted = true;
          startGame(game, human, status, pendingComputerMove, computerThinking, lastComputerMove,
                    currentSearchOptions());
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
    (void)boardClickToMove(game, human, gameStarted, status, pendingComputerMove, computerThinking,
                           currentSearchOptions());

    BeginDrawing();
    ClearBackground(Color{240, 242, 245, 255});
    drawBoard(game, lastComputerMove);

    DrawRectangle(static_cast<int>(kPanelLeft), static_cast<int>(kPanelTop),
                  static_cast<int>(kPanelWidth), static_cast<int>(kPanelHeight), kPanelColor);
    if (computerThinking) {
      drawThinking(kPanelContentLeft, 33.0F);
    } else {
      DrawText(status.c_str(), static_cast<int>(kPanelContentLeft), 33, 22, RAYWHITE);
    }
    const Piece winner = game.checkWinner();
    const char* hint = !gameStarted ? "Black moves first."
                       : winner != Piece::EMPTY || game.isBoardFull()
                           ? "Start a new game to play again."
                       : computerThinking ? "Your move is on the board."
                                          : "Click board to play.";
    DrawText(hint, static_cast<int>(kPanelContentLeft), 69, 17, Color{185, 194, 202, 255});
    drawEffortSelector(effort, !computerThinking);
    drawSeedEditor(seedBounds, seedText, seedFieldFocused, seedTextSelected, seedTextInvalid,
                   GetTime() < seedCopiedUntil, !computerThinking);
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
