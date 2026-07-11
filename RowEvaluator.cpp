#include "RowEvaluator.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include "BitRowBuilder.h"

namespace gomoku {

RowEvaluator::RowEvaluator()
{
  reset();
}

void RowEvaluator::reset()
{
  patterns.clear();
  tables = std::make_shared<EvaluationTables>();
  tables->oddStep.fill(-1);
  tables->evenStep.fill(-1);
}

RowEvaluator::RowDp RowEvaluator::initDP()
{
  RowDp dp{};
  dp.fill(-1);
  return dp;
}

int RowEvaluator::rowDP(std::uint16_t row, RowDp& dp, int index, bool type)
{
  int currentMax = 0;
  BitRowBuilder rowBuilder(row);
  if (rowBuilder.length() < 2) {
    dp[index] = 0;
    return 0;
  }
  if (dp[index] != -1) {
    return dp[index];
  }

  if (!rowBuilder.isSet(0) && !rowBuilder.isSet(1)) {
    return rowDP(BitRowBuilder::RowSplice(row, 1), dp, index + 1, type);
  }

  for (const PatternEvaluation& pattern : patterns) {
    const int patternEvaluation = type ? pattern.oddStep : pattern.evenStep;
    if (BitRowBuilder::RowStartsWith(row, pattern.pattern)) {
      const int value = patternEvaluation + rowDP(BitRowBuilder::RowSplice(row, pattern.length), dp,
                                                  index + pattern.length, type);
      currentMax = std::max(value, currentMax);
    }
    if (BitRowBuilder::RowStartsWith(row, pattern.reverse)) {
      const int value = patternEvaluation + rowDP(BitRowBuilder::RowSplice(row, pattern.length), dp,
                                                  index + pattern.length, type);
      currentMax = std::max(value, currentMax);
    }
  }

  const int value = rowDP(BitRowBuilder::RowSplice(row, 1), dp, index + 1, type);
  currentMax = std::max(value, currentMax);
  dp[index] = currentMax;
  return currentMax;
}

int RowEvaluator::genEvalForRow(std::uint16_t row, bool type)
{
  const auto saveResult = [this, row, type](int value) {
    (type ? tables->oddStep : tables->evenStep)[static_cast<std::size_t>(row)] = value;
    return value;
  };

  if (row <= (1 << 5)) {
    return saveResult(0);
  }
  for (int i = 1; i < 17; ++i) {
    if (row == (1 << i)) {
      return saveResult(0);
    }
  }

  const std::uint16_t reverseRow = BitRowBuilder::GetReverse(row);
  const auto& evaluations = type ? tables->oddStep : tables->evenStep;
  if (evaluations[static_cast<std::size_t>(reverseRow)] != -1) {
    return saveResult(evaluations[static_cast<std::size_t>(reverseRow)]);
  }

  auto forwardDP = initDP();
  auto backwardDP = initDP();
  const int forwardEval = rowDP(row, forwardDP, 0, type);
  const int backwardEval = rowDP(reverseRow, backwardDP, 0, type);
  return saveResult(std::max(forwardEval, backwardEval));
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
    int eval1 = 0;
    int eval2 = 0;
    if (!(lineStream >> pattern >> eval1 >> eval2)) {
      continue;
    }

    if (pattern.size() > kMaximumEncodedRowLength ||
        !std::all_of(pattern.begin(), pattern.end(),
                     [](char bit) { return bit == '0' || bit == '1'; })) {
      throw std::runtime_error("Invalid pattern in file: " + pattern);
    }

    pattern.insert(pattern.begin(), '1');
    const auto encodedPattern = static_cast<std::uint16_t>(std::stoul(pattern, nullptr, 2));
    const auto existing = std::find_if(
        patterns.begin(), patterns.end(),
        [encodedPattern](const auto& value) { return value.pattern == encodedPattern; });
    const PatternEvaluation value{
        .pattern = encodedPattern,
        .reverse = BitRowBuilder::GetReverse(encodedPattern),
        .length = static_cast<std::uint8_t>(BitRowBuilder::LengthOf(encodedPattern)),
        .oddStep = eval1,
        .evenStep = eval2};
    if (existing == patterns.end()) {
      patterns.push_back(value);
    } else {
      *existing = value;
    }
  }

  for (int row = 0; row < (1 << 16); ++row) {
    const auto encodedRow = static_cast<std::uint16_t>(row);
    (void)genEvalForRow(encodedRow, true);
    (void)genEvalForRow(encodedRow, false);
  }

  return tables;
}

}  // namespace gomoku
