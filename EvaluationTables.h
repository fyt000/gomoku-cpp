#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

namespace gomoku {

inline constexpr std::size_t kEncodedRowCount = 1u << 16;
inline constexpr std::size_t kMaximumEncodedRowLength = 15;

enum class PatternTurn : std::uint8_t {
  OWNER_TO_MOVE,
  OPPONENT_TO_MOVE,
};

struct EvaluationTables {
  std::array<std::int32_t, kEncodedRowCount> opponentToMove{};
  std::array<std::int32_t, kEncodedRowCount> ownerToMove{};
};

using EvaluationTablesPtr = std::shared_ptr<const EvaluationTables>;

}  // namespace gomoku
