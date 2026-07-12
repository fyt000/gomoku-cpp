#include "RowEvaluator.h"

#include <algorithm>
#include <cctype>
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
  std::size_t lineNumber = 0;
  while (std::getline(input, line)) {
    ++lineNumber;
    const auto isWhitespace = [](char character) {
      return std::isspace(static_cast<unsigned char>(character)) != 0;
    };
    const auto firstContent = std::find_if_not(line.begin(), line.end(), isWhitespace);
    if (firstContent == line.end() || *firstContent == '#') {
      continue;
    }

    std::string patternDefinition = line;
    const std::size_t comment = patternDefinition.find('#');
    if (comment != std::string::npos) {
      if (comment == 0 || !isWhitespace(patternDefinition[comment - 1])) {
        throw std::runtime_error("Invalid pattern line " + std::to_string(lineNumber) +
                                 " in " + patternFile + ": " + line);
      }
      patternDefinition.erase(comment);
    }

    std::istringstream lineStream(patternDefinition);
    std::string pattern;
    int opponentToMove = 0;
    int ownerToMove = 0;
    if (!(lineStream >> pattern >> opponentToMove >> ownerToMove)) {
      throw std::runtime_error("Invalid pattern line " + std::to_string(lineNumber) +
                               " in " + patternFile + ": " + line);
    }
    lineStream >> std::ws;

    if (pattern.size() > kMaximumEncodedRowLength ||
        !std::all_of(pattern.begin(), pattern.end(),
                     [](char bit) { return bit == '0' || bit == '1'; }) ||
        opponentToMove < 0 || ownerToMove < 0 || !lineStream.eof()) {
      throw std::runtime_error("Invalid pattern line " + std::to_string(lineNumber) +
                               " in " + patternFile + ": " + line);
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
