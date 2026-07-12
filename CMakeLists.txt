cmake_minimum_required(VERSION 3.21)

project(gomoku-cpp VERSION 1.0.0 LANGUAGES CXX)

include(CTest)
include(CheckIPOSupported)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
option(GOMOKU_ENABLE_IPO "Enable interprocedural optimization in Release builds" OFF)

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/pattern.txt"
)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/pattern.txt" gomoku_pattern_lines)
set(gomoku_pattern_entries "")
set(gomoku_pattern_count 0)
set(gomoku_seen_patterns "")
foreach(gomoku_pattern_line IN LISTS gomoku_pattern_lines)
  string(STRIP "${gomoku_pattern_line}" gomoku_pattern_line)
  if(gomoku_pattern_line STREQUAL "" OR gomoku_pattern_line MATCHES "^#")
    continue()
  endif()
  if(NOT gomoku_pattern_line MATCHES
     "^([01]+)[ \\t]+([0-9]+)[ \\t]+([0-9]+)[ \\t]*$")
    message(FATAL_ERROR "Invalid pattern line: ${gomoku_pattern_line}")
  endif()
  string(LENGTH "${CMAKE_MATCH_1}" gomoku_pattern_length)
  if(gomoku_pattern_length GREATER 15)
    message(FATAL_ERROR "Pattern is longer than the encoded row: ${CMAKE_MATCH_1}")
  endif()
  if(CMAKE_MATCH_1 IN_LIST gomoku_seen_patterns)
    message(FATAL_ERROR "Duplicate pattern: ${CMAKE_MATCH_1}")
  endif()
  list(APPEND gomoku_seen_patterns "${CMAKE_MATCH_1}")
  string(APPEND gomoku_pattern_entries
    "    PatternSource{\"${CMAKE_MATCH_1}\", ${CMAKE_MATCH_2}, ${CMAKE_MATCH_3}},\n")
  math(EXPR gomoku_pattern_count "${gomoku_pattern_count} + 1")
endforeach()

set(gomoku_generated_directory "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${gomoku_generated_directory}")
set(gomoku_pattern_header "${gomoku_generated_directory}/PatternDefinitions.generated.h")
set(gomoku_pattern_header_temporary "${gomoku_pattern_header}.tmp")
file(WRITE "${gomoku_pattern_header_temporary}"
"#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace gomoku::compiled_detail {

struct PatternSource {
  std::string_view pattern;
  std::int32_t opponentToMove;
  std::int32_t ownerToMove;
};

inline constexpr std::array<PatternSource, ${gomoku_pattern_count}> kPatternSources{{
${gomoku_pattern_entries}}};

}  // namespace gomoku::compiled_detail
")
configure_file("${gomoku_pattern_header_temporary}" "${gomoku_pattern_header}" COPYONLY)
file(REMOVE "${gomoku_pattern_header_temporary}")

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
  "${CMAKE_CURRENT_SOURCE_DIR}/THIRD_PARTY_NOTICES.txt"
)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE" GOMOKU_PROJECT_LICENSE)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/THIRD_PARTY_NOTICES.txt"
  GOMOKU_THIRD_PARTY_NOTICES)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/LicenseNotices.cpp.in"
  "${gomoku_generated_directory}/LicenseNotices.cpp"
  @ONLY
)

add_library(gomoku_engine STATIC
  BitRowBuilder.cpp
  Board.cpp
  CompiledEvaluationTables.cpp
  Gomoku.cpp
  "${gomoku_generated_directory}/LicenseNotices.cpp"
  RowEvaluator.cpp
)

target_compile_features(gomoku_engine PUBLIC cxx_std_20)
target_include_directories(gomoku_engine PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
target_include_directories(gomoku_engine PRIVATE "${gomoku_generated_directory}")

if(MSVC)
  target_compile_options(gomoku_engine PRIVATE /constexpr:steps100000000)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(gomoku_engine PRIVATE -fconstexpr-steps=100000000)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_compile_options(gomoku_engine PRIVATE -fconstexpr-ops-limit=100000000)
endif()

if(GOMOKU_ENABLE_IPO)
  check_ipo_supported(RESULT gomoku_ipo_supported OUTPUT gomoku_ipo_error)
else()
  set(gomoku_ipo_supported OFF)
endif()

function(gomoku_enable_ipo target)
  if(gomoku_ipo_supported)
    set_property(TARGET ${target} PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
  endif()
endfunction()

add_library(gomoku_session STATIC GameSession.cpp)
target_link_libraries(gomoku_session PUBLIC gomoku_engine)
target_compile_features(gomoku_session PUBLIC cxx_std_20)

add_executable(gomoku-cpp GomokuDriver.cpp)
target_link_libraries(gomoku-cpp PRIVATE gomoku_engine)

find_package(httplib CONFIG REQUIRED)
find_package(glaze CONFIG REQUIRED)
find_package(raylib CONFIG REQUIRED)
find_package(Threads REQUIRED)
target_link_libraries(gomoku_engine PUBLIC Threads::Threads)

add_executable(gomoku-server GomokuServer.cpp)
target_link_libraries(gomoku-server PRIVATE
  gomoku_engine
  httplib::httplib
  glaze::glaze
)

add_executable(gomoku-gui GomokuGui.cpp)
target_link_libraries(gomoku-gui PRIVATE gomoku_session raylib)

add_executable(gomoku-mcp GomokuMcp.cpp)
target_compile_features(gomoku-mcp PRIVATE cxx_std_23)
target_link_libraries(gomoku-mcp PRIVATE gomoku_session glaze::glaze)

function(gomoku_enable_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endfunction()

gomoku_enable_warnings(gomoku_engine)
gomoku_enable_warnings(gomoku_session)
gomoku_enable_warnings(gomoku-cpp)
gomoku_enable_warnings(gomoku-server)
gomoku_enable_warnings(gomoku-gui)
gomoku_enable_warnings(gomoku-mcp)

gomoku_enable_ipo(gomoku_engine)
gomoku_enable_ipo(gomoku_session)
gomoku_enable_ipo(gomoku-cpp)
gomoku_enable_ipo(gomoku-server)
gomoku_enable_ipo(gomoku-gui)
gomoku_enable_ipo(gomoku-mcp)

if(BUILD_TESTING)
  find_package(Catch2 3 CONFIG REQUIRED)

  add_executable(gomoku-tests tests/CoreTests.cpp)
  target_link_libraries(gomoku-tests PRIVATE gomoku_session Catch2::Catch2WithMain)
  target_compile_definitions(gomoku-tests PRIVATE GOMOKU_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
  gomoku_enable_warnings(gomoku-tests)
  gomoku_enable_ipo(gomoku-tests)

  add_test(NAME gomoku-tests COMMAND gomoku-tests)
endif()
