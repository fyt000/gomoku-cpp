#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

namespace gomoku {

inline constexpr std::size_t kEncodedRowCount = 1u << 16;
inline constexpr std::size_t kMaximumEncodedRowLength = 15;

struct EvaluationTables {
  std::array<std::int32_t, kEncodedRowCount> oddStep{};
  std::array<std::int32_t, kEncodedRowCount> evenStep{};
};

using EvaluationTablesPtr = std::shared_ptr<const EvaluationTables>;

}  // namespace gomoku
