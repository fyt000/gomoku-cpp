#pragma once

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "EvaluationTables.h"

namespace gomoku {

struct EvaluationPattern {
  std::uint16_t pattern = 0;
  std::uint16_t reverse = 0;
  std::uint8_t length = 0;
  std::int32_t opponentToMove = 0;
  std::int32_t ownerToMove = 0;
};

namespace pattern_cache {

constexpr int rowLength(std::uint16_t row)
{
  return row == 0 ? 0 : static_cast<int>(std::bit_width(row)) - 1;
}

constexpr std::uint16_t reverseRow(std::uint16_t row)
{
  const int length = rowLength(row);
  std::uint16_t reversed = 1;
  for (int index = 0; index < length; ++index) {
    reversed = static_cast<std::uint16_t>(
        (reversed << 1) | ((row & (std::uint16_t{1} << index)) != 0 ? 1 : 0));
  }
  return reversed;
}

constexpr std::uint16_t encodePattern(std::string_view pattern)
{
  std::uint16_t encoded = 1;
  for (const char bit : pattern) {
    encoded = static_cast<std::uint16_t>((encoded << 1) | (bit == '1' ? 1 : 0));
  }
  return encoded;
}

constexpr EvaluationPattern makePattern(std::string_view pattern, std::int32_t opponentToMove,
                                        std::int32_t ownerToMove)
{
  const std::uint16_t encoded = encodePattern(pattern);
  return {
      .pattern = encoded,
      .reverse = reverseRow(encoded),
      .length = static_cast<std::uint8_t>(pattern.size()),
      .opponentToMove = opponentToMove,
      .ownerToMove = ownerToMove,
  };
}

constexpr std::uint16_t rowSplice(std::uint16_t row, int index)
{
  const int remaining = rowLength(row) - index;
  const std::uint16_t remainingMask =
      remaining == 0 ? 0 : static_cast<std::uint16_t>((std::uint32_t{1} << remaining) - 1);
  return static_cast<std::uint16_t>((row & remainingMask) | (std::uint16_t{1} << remaining));
}

struct RowEvaluation {
  std::int32_t opponentToMove = 0;
  std::int32_t ownerToMove = 0;
};

template <typename PatternRange>
constexpr RowEvaluation evaluateSuffix(const PatternRange& patterns,
                                       const EvaluationTables& suffixTables, std::uint16_t row)
{
  const int length = rowLength(row);
  if (length < 2) {
    return {};
  }

  const std::size_t skipped = rowSplice(row, 1);
  RowEvaluation best{
      .opponentToMove = suffixTables.opponentToMove[skipped],
      .ownerToMove = suffixTables.ownerToMove[skipped],
  };

  for (const EvaluationPattern& pattern : patterns) {
    if (pattern.length > length) {
      continue;
    }
    const std::uint16_t prefix = static_cast<std::uint16_t>(row >> (length - pattern.length));
    if (prefix != pattern.pattern && prefix != pattern.reverse) {
      continue;
    }

    const std::size_t remaining = rowSplice(row, pattern.length);
    best.opponentToMove =
        std::max(best.opponentToMove,
                 pattern.opponentToMove + suffixTables.opponentToMove[remaining]);
    best.ownerToMove =
        std::max(best.ownerToMove, pattern.ownerToMove + suffixTables.ownerToMove[remaining]);
  }

  return best;
}

template <typename PatternRange>
constexpr void generate(const PatternRange& patterns, EvaluationTables& suffixTables,
                        EvaluationTables& tables)
{
  suffixTables.opponentToMove.fill(0);
  suffixTables.ownerToMove.fill(0);
  tables.opponentToMove.fill(0);
  tables.ownerToMove.fill(0);
  for (std::size_t row = 0; row < kEncodedRowCount; ++row) {
    const auto encoded = static_cast<std::uint16_t>(row);
    const RowEvaluation evaluation = evaluateSuffix(patterns, suffixTables, encoded);
    suffixTables.opponentToMove[row] = evaluation.opponentToMove;
    suffixTables.ownerToMove[row] = evaluation.ownerToMove;
    if (encoded > (1 << 5) && !std::has_single_bit(encoded)) {
      tables.opponentToMove[row] = evaluation.opponentToMove;
      tables.ownerToMove[row] = evaluation.ownerToMove;
    }
  }
}

}  // namespace pattern_cache

}  // namespace gomoku
