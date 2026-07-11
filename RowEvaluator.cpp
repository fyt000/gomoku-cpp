#include "RowEvaluator.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include "BitRowBuilder.h"

namespace gomoku {

RowEvaluator::RowEvaluator() : rowEval1(1 << 16, -1), rowEval2(1 << 16, -1) {}

std::vector<int> RowEvaluator::initDP(int row) const
{
  std::vector<int> dp(BitRowBuilder::LengthOf(row), -1);
  dp.insert(dp.end(), maxPatLen, 0);
  return dp;
}

int RowEvaluator::rowDP(int row, std::vector<int>& dp, int index, bool type)
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

  const std::map<int, int>& evaluationMap = type ? patternEvalMap1 : patternEvalMap2;
  for (const auto& [pattern, patternEvaluation] : evaluationMap) {
    const int patternLength = BitRowBuilder::LengthOf(pattern);
    if (BitRowBuilder::RowStartsWith(row, pattern)) {
      const int value = patternEvaluation + rowDP(BitRowBuilder::RowSplice(row, patternLength), dp,
                                                  index + patternLength, type);
      currentMax = std::max(value, currentMax);
    }
    const int reversePattern = BitRowBuilder::GetReverse(pattern);
    if (BitRowBuilder::RowStartsWith(row, reversePattern)) {
      const int value = patternEvaluation + rowDP(BitRowBuilder::RowSplice(row, patternLength), dp,
                                                  index + patternLength, type);
      currentMax = std::max(value, currentMax);
    }
  }

  const int value = rowDP(BitRowBuilder::RowSplice(row, 1), dp, index + 1, type);
  currentMax = std::max(value, currentMax);
  dp[index] = currentMax;
  return currentMax;
}

int RowEvaluator::genEvalForRow(int row, bool type)
{
  const auto saveResult = [this, row, type](int value) {
    (type ? rowEval1 : rowEval2)[row] = value;
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

  const int reverseRow = BitRowBuilder::GetReverse(row);
  const std::vector<int>& evaluations = type ? rowEval1 : rowEval2;
  if (evaluations[reverseRow] != -1) {
    return saveResult(evaluations[reverseRow]);
  }

  auto forwardDP = initDP(row);
  auto backwardDP = initDP(reverseRow);
  const int forwardEval = rowDP(row, forwardDP, 0, type);
  const int backwardEval = rowDP(reverseRow, backwardDP, 0, type);
  return saveResult(std::max(forwardEval, backwardEval));
}

void RowEvaluator::setPatterns(const std::string& patternFile, std::vector<int>& outputRowEval1,
                               std::vector<int>& outputRowEval2)
{
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

    pattern.insert(pattern.begin(), '1');
    maxPatLen = std::max(maxPatLen, static_cast<int>(pattern.size()) - 1);
    const int encodedPattern = std::stoi(pattern, nullptr, 2);
    patternEvalMap1[encodedPattern] = eval1;
    patternEvalMap2[encodedPattern] = eval2;
  }

  for (int row = 0; row < (1 << 16); ++row) {
    (void)genEvalForRow(row, true);
    (void)genEvalForRow(row, false);
  }

  outputRowEval1 = rowEval1;
  outputRowEval2 = rowEval2;
}

}  // namespace gomoku
