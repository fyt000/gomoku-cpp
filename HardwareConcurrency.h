#pragma once

namespace gomoku {

// Returns the number of CPU cores suitable for independent search workers.
// On Windows this excludes simultaneous multithreading siblings; elsewhere it
// falls back to the logical CPU count exposed by standard C++.
[[nodiscard]] unsigned int availableSearchThreadCount();

}  // namespace gomoku
