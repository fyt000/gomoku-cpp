#include "CompiledEvaluationTables.h"

#include <array>
#include <cstddef>
#include <memory>

#include "PatternDefinitions.generated.h"
#include "PatternCacheGenerator.h"

namespace gomoku {

namespace {

using compiled_detail::kPatternSources;

consteval auto compilePatterns()
{
  std::array<EvaluationPattern, kPatternSources.size()> patterns{};
  for (std::size_t index = 0; index < patterns.size(); ++index) {
    const auto& source = kPatternSources[index];
    patterns[index] =
        pattern_cache::makePattern(source.pattern, source.opponentToMove, source.ownerToMove);
  }
  return patterns;
}

inline constexpr auto kPatterns = compilePatterns();

consteval EvaluationTables generateEvaluationTables()
{
  EvaluationTables suffixTables;
  EvaluationTables tables;
  pattern_cache::generate(kPatterns, suffixTables, tables);
  return tables;
}

inline constexpr EvaluationTables kCompiledTables = generateEvaluationTables();

}  // namespace

const EvaluationTablesPtr& compiledEvaluationTables()
{
  static const EvaluationTablesPtr tables(&kCompiledTables, [](const EvaluationTables*) {});
  return tables;
}

}  // namespace gomoku
