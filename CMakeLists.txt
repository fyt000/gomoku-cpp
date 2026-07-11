cmake_minimum_required(VERSION 3.21)

project(gomoku-cpp VERSION 1.0.0 LANGUAGES CXX)

include(CTest)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(gomoku_engine STATIC
  BitRowBuilder.cpp
  Board.cpp
  Gomoku.cpp
  RowEvaluator.cpp
)

target_compile_features(gomoku_engine PUBLIC cxx_std_20)
target_include_directories(gomoku_engine PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

add_library(gomoku_session STATIC GameSession.cpp)
target_link_libraries(gomoku_session PUBLIC gomoku_engine)
target_compile_features(gomoku_session PUBLIC cxx_std_20)

add_executable(gomoku-cpp GomokuDriver.cpp)
target_link_libraries(gomoku-cpp PRIVATE gomoku_engine)

find_package(httplib CONFIG REQUIRED)
find_package(glaze CONFIG REQUIRED)
find_package(raylib CONFIG REQUIRED)

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

if(BUILD_TESTING)
  find_package(Catch2 3 CONFIG REQUIRED)

  add_executable(gomoku-tests tests/CoreTests.cpp)
  target_link_libraries(gomoku-tests PRIVATE gomoku_session Catch2::Catch2WithMain)
  gomoku_enable_warnings(gomoku-tests)

  add_test(NAME gomoku-tests COMMAND gomoku-tests)
endif()
