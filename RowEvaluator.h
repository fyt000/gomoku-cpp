#pragma once

#include <vector>
#include <map>
#include <string>

class RowEvaluator
{
public:
  RowEvaluator();
  void setPatterns(const std::string &patternFile, std::vector<int> &retRowEval1, std::vector<int> &retRowEval2);

private:
  int genEvalForRow(int row, bool type);
  int rowDP(int row, std::vector<int> &dp, int idx, bool type);
  std::vector<int> initDP(int row);

  int maxPatLen = 0;

  std::map<int, int> patternEvalMap1;
  std::map<int, int> patternEvalMap2;

  std::vector<int> rowEval1;
  std::vector<int> rowEval2;
};