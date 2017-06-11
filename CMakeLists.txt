project (gomoku-cpp)

set(CMAKE_CXX_FLAGS "-O2 -std=c++14 -MD")

add_executable(gomoku-cpp Board.cpp BitRowBuilder.cpp Gomoku.cpp GomokuDriver.cpp)

