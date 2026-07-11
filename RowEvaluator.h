#pragma once

#include <map>
#include <string>
#include <vector>

namespace gomoku {

class RowEvaluator {
 public:
  RowEvaluator();
  void setPatterns(const std::string& patternFile, std::vector<int>& rowEval1,
                   std::vector<int>& rowEval2);

 private:
  [[nodiscard]] int genEvalForRow(int row, bool type);
  [[nodiscard]] int rowDP(int row, std::vector<int>& dp, int index, bool type);
  [[nodiscard]] std::vector<int> initDP(int row) const;

  int maxPatLen = 0;
  std::map<int, int> patternEvalMap1;
  std::map<int, int> patternEvalMap2;
  std::vector<int> rowEval1;
  std::vector<int> rowEval2;
};

}  // namespace gomoku
