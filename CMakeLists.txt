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

# Native optimizations (opt-in for maximum performance)
option(ENABLE_NATIVE_OPTIMIZATIONS "Enable -march=native, -flto, and aggressive optimizations" OFF)

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

if(ENABLE_NATIVE_OPTIMIZATIONS AND NOT MSVC)
    target_compile_options(ttt PRIVATE -march=native -funroll-loops -fomit-frame-pointer -fno-semantic-interposition)
    target_link_options(ttt PRIVATE -flto)
    set_target_properties(ttt PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
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

if(ENABLE_NATIVE_OPTIMIZATIONS AND NOT MSVC)
    target_compile_options(test_runner PRIVATE -march=native -funroll-loops -fomit-frame-pointer -fno-semantic-interposition)
    target_link_options(test_runner PRIVATE -flto)
    set_target_properties(test_runner PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

enable_testing()
add_test(NAME unit_tests COMMAND test_runner)
