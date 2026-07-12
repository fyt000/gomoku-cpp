#pragma once

#include <algorithm>
#include <array>
#include <optional>
#include <string_view>

#include "Gomoku.h"
#include "HardwareConcurrency.h"

namespace gomoku {

enum class SearchEffort : int { LIGHT, MEDIUM, HIGH, MAX, ULTRA };

struct SearchEffortPreset {
  std::string_view id;
  std::string_view displayName;
  int depth;
  int frontierRadius;
  unsigned int maximumThreads;
};

inline constexpr std::array kSearchEffortPresets{
    SearchEffortPreset{"light", "Light", 2, 1, 1},
    SearchEffortPreset{"medium", "Medium", 3, 2, 1},
    SearchEffortPreset{"high", "High", 5, 2, 1},
    SearchEffortPreset{"max", "Max", 6, 2, 4},
    SearchEffortPreset{"ultra", "Ultra", 6, 3, 8},
};

inline constexpr const SearchEffortPreset& searchEffortPreset(SearchEffort effort)
{
  return kSearchEffortPresets[static_cast<int>(effort)];
}

inline std::optional<SearchEffort> parseSearchEffort(std::string_view name)
{
  for (std::size_t index = 0; index < kSearchEffortPresets.size(); ++index) {
    if (kSearchEffortPresets[index].id == name) {
      return static_cast<SearchEffort>(index);
    }
  }
  return std::nullopt;
}

inline SearchOptions searchOptionsForEffort(SearchEffort effort)
{
  const SearchEffortPreset& preset = searchEffortPreset(effort);
  return {.depth = preset.depth,
          .frontierRadius = preset.frontierRadius,
          .threadCount = std::min(preset.maximumThreads, availableSearchThreadCount())};
}

}  // namespace gomoku
