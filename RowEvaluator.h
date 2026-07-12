#pragma once

#include <memory>
#include <string>
#include <vector>

#include "EvaluationTables.h"
#include "PatternCacheGenerator.h"

namespace gomoku {

class RowEvaluator {
 public:
  RowEvaluator();
 [[nodiscard]] EvaluationTablesPtr loadPatterns(const std::string& patternFile);

 private:
  void reset();

  std::vector<EvaluationPattern> patterns;
  std::shared_ptr<EvaluationTables> tables;
};

}  // namespace gomoku
