#include "RowEvaluator.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace gomoku {

RowEvaluator::RowEvaluator()
{
  reset();
}

void RowEvaluator::reset()
{
  patterns.clear();
  tables = std::make_shared<EvaluationTables>();
}

EvaluationTablesPtr RowEvaluator::loadPatterns(const std::string& patternFile)
{
  reset();
  std::ifstream input(patternFile);
  if (!input) {
    throw std::runtime_error("Unable to open pattern file: " + patternFile);
  }

  std::string line;
  while (std::getline(input, line)) {
    std::istringstream lineStream(line);
    std::string pattern;
    int opponentToMove = 0;
    int ownerToMove = 0;
    if (!(lineStream >> pattern >> opponentToMove >> ownerToMove)) {
      continue;
    }

    if (pattern.size() > kMaximumEncodedRowLength ||
        !std::all_of(pattern.begin(), pattern.end(),
                     [](char bit) { return bit == '0' || bit == '1'; })) {
      throw std::runtime_error("Invalid pattern in file: " + pattern);
    }

    const EvaluationPattern value =
        pattern_cache::makePattern(pattern, opponentToMove, ownerToMove);
    const auto existing = std::find_if(
        patterns.begin(), patterns.end(), [&value](const auto& existingPattern) {
          return existingPattern.pattern == value.pattern;
        });
    if (existing == patterns.end()) {
      patterns.push_back(value);
    } else {
      *existing = value;
    }
  }

  auto suffixTables = std::make_unique<EvaluationTables>();
  pattern_cache::generate(patterns, *suffixTables, *tables);

  return tables;
}

}  // namespace gomoku
