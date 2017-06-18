cmake_minimum_required(VERSION 3.7)

project (gomoku-cpp C CXX)

find_library(CPPREST_LIB cpprest)
find_package(Boost REQUIRED COMPONENTS random system thread filesystem chrono atomic date_time regex)
find_package(OpenSSL 1.0.0 REQUIRED)
find_package (Threads REQUIRED)

set(CMAKE_CXX_FLAGS "-O2 -std=c++14 -MD")

add_executable(gomoku-cpp Board.cpp BitRowBuilder.cpp Gomoku.cpp GomokuDriver.cpp RowEvaluator.cpp)
target_link_libraries(gomoku-cpp
  ${CMAKE_THREAD_LIBS_INIT}
)


add_executable(gomoku-server GomokuServer.cpp Board.cpp BitRowBuilder.cpp Gomoku.cpp RowEvaluator.cpp)

target_link_libraries(gomoku-server
  ${CPPREST_LIB}
  Boost::boost
  Boost::random
  Boost::system
  Boost::thread
  Boost::filesystem
  Boost::chrono
  Boost::atomic
  Boost::date_time
  Boost::regex
  OpenSSL::SSL
  ${CMAKE_THREAD_LIBS_INIT}
)