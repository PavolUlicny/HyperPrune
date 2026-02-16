cmake_minimum_required(VERSION 3.10)
project(HyperPrune LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Board size (3-8)
set(BOARD_SIZE "3" CACHE STRING "Board size (3-8)")
if(BOARD_SIZE LESS 3 OR BOARD_SIZE GREATER 8)
    message(FATAL_ERROR "BOARD_SIZE must be between 3 and 8 (got ${BOARD_SIZE})")
endif()

# Compiler warnings
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra)
endif()

# Main executable
add_executable(ttt
    src/main.c
    src/TicTacToe/tic_tac_toe.c
    src/MiniMax/mini_max.c
    src/MiniMax/transposition.c
)
target_compile_definitions(ttt PRIVATE BOARD_SIZE=${BOARD_SIZE})

if(NOT MSVC)
    target_link_libraries(ttt m)
endif()

# Test executable
add_executable(test_runner
    test/test_runner.c
    test/test_bitboard.c
    test/test_minimax.c
    test/test_zobrist.c
    test/test_transposition_table.c
    test/test_game_scenarios.c
    test/test_edge_cases.c
    test/test_correctness.c
    test/unity/unity.c
    src/TicTacToe/tic_tac_toe.c
    src/MiniMax/mini_max.c
    src/MiniMax/transposition.c
)
target_compile_definitions(test_runner PRIVATE BOARD_SIZE=${BOARD_SIZE})
target_include_directories(test_runner PRIVATE test/unity)

if(NOT MSVC)
    target_link_libraries(test_runner m)
endif()

enable_testing()
add_test(NAME unit_tests COMMAND test_runner)
