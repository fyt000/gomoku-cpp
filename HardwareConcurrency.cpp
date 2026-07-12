#include "HardwareConcurrency.h"

#include <algorithm>
#include <thread>

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>

#include <cstddef>
#include <vector>
#endif

namespace gomoku {

namespace {

unsigned int logicalProcessorCount()
{
  return std::max(1u, std::thread::hardware_concurrency());
}

#ifdef _WIN32
unsigned int physicalCoreCount()
{
  DWORD bufferSize = 0;
  if (GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &bufferSize) != FALSE ||
      GetLastError() != ERROR_INSUFFICIENT_BUFFER || bufferSize == 0) {
    return 0;
  }

  std::vector<std::byte> buffer(bufferSize);
  auto* information =
      reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer.data());
  if (GetLogicalProcessorInformationEx(RelationProcessorCore, information, &bufferSize) == FALSE) {
    return 0;
  }

  unsigned int cores = 0;
  DWORD offset = 0;
  while (offset < bufferSize) {
    const auto* entry = reinterpret_cast<const SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(
        buffer.data() + offset);
    if (entry->Size == 0 || entry->Relationship != RelationProcessorCore) {
      return 0;
    }
    ++cores;
    offset += entry->Size;
  }
  return cores;
}
#endif

}  // namespace

unsigned int availableSearchThreadCount()
{
  static const unsigned int count = [] {
#ifdef _WIN32
    if (const unsigned int cores = physicalCoreCount(); cores != 0) {
      return cores;
    }
#endif
    return logicalProcessorCount();
  }();
  return count;
}

}  // namespace gomoku
