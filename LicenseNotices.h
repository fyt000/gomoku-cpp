#pragma once

namespace gomoku {

// Prints the project and third-party licenses to stdout when the sole argument
// is --licenses. Returns true when the argument was handled.
bool printLicenseNoticesIfRequested(int argc, char** argv);

}  // namespace gomoku
