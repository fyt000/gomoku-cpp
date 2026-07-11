#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "EvaluationTables.h"

namespace gomoku {

class RowEvaluator {
 public:
  RowEvaluator();
  [[nodiscard]] EvaluationTablesPtr loadPatterns(const std::string& patternFile);

 private:
  struct PatternEvaluation {
    std::uint16_t pattern = 0;
    std::uint16_t reverse = 0;
    std::uint8_t length = 0;
    std::int32_t oddStep = 0;
    std::int32_t evenStep = 0;
  };

  using RowDp = std::array<int, kMaximumEncodedRowLength + 1>;

  [[nodiscard]] int genEvalForRow(std::uint16_t row, bool type);
  [[nodiscard]] int rowDP(std::uint16_t row, RowDp& dp, int index, bool type);
  [[nodiscard]] static RowDp initDP();
  void reset();

  std::vector<PatternEvaluation> patterns;
  std::shared_ptr<EvaluationTables> tables;
};

}  // namespace gomoku
