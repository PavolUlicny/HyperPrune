cmake_minimum_required(VERSION 3.15)
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

# CPU-specific tuning (opt-in): adds -march=native to target the build machine's CPU.
# LTO, -funroll-loops, and -fno-semantic-interposition are always applied to Release builds.
option(ENABLE_NATIVE_OPTIMIZATIONS "Enable -march=native for CPU-specific tuning" OFF)

# Move ordering: killer-move and history heuristics.
# Default: ON for BOARD_SIZE <= 4 (ordering improves pruning), OFF for larger boards
# (O(n^2) sort overhead outweighs pruning gains when TT coverage drops).
if(BOARD_SIZE LESS 5)
    set(ENABLE_MOVE_ORDERING_DEFAULT ON)
else()
    set(ENABLE_MOVE_ORDERING_DEFAULT OFF)
endif()
option(ENABLE_MOVE_ORDERING "Enable killer-move and history heuristics for move ordering" ${ENABLE_MOVE_ORDERING_DEFAULT})

# Main executable
add_executable(ttt
    src/main.c
    src/TicTacToe/tic_tac_toe.c
    src/Negamax/negamax.c
    src/Negamax/transposition.c
)
target_compile_definitions(ttt PRIVATE BOARD_SIZE=${BOARD_SIZE})
target_compile_definitions(ttt PRIVATE ENABLE_MOVE_ORDERING=$<BOOL:${ENABLE_MOVE_ORDERING}>)

# Static CRT for portable Windows distribution (no Visual C++ Redistributable required)
if(MSVC)
    set_target_properties(ttt PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )
endif()

if(NOT MSVC)
    target_link_libraries(ttt m)
endif()

# Portable Release flags: no -march=native, compatible with any CPU
if(NOT MSVC)
    target_compile_options(ttt PRIVATE
        $<$<CONFIG:Release>:-funroll-loops>
        $<$<CONFIG:Release>:-fomit-frame-pointer>
    )
    if(NOT APPLE)
        target_compile_options(ttt PRIVATE $<$<CONFIG:Release>:-fno-semantic-interposition>)
    endif()
endif()

# LTO for Release builds
include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported)
if(ipo_supported)
    set_target_properties(ttt PROPERTIES INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
endif()

if(ENABLE_NATIVE_OPTIMIZATIONS AND NOT MSVC)
    target_compile_options(ttt PRIVATE -march=native)
endif()

# Test executable
add_executable(test_runner
    test/test_runner.c
    test/test_bitboard.c
    test/test_negamax.c
    test/test_zobrist.c
    test/test_transposition_table.c
    test/test_game_scenarios.c
    test/test_edge_cases.c
    test/test_correctness.c
    test/unity/unity.c
    src/TicTacToe/tic_tac_toe.c
    src/Negamax/negamax.c
    src/Negamax/transposition.c
)
target_compile_definitions(test_runner PRIVATE BOARD_SIZE=${BOARD_SIZE})
target_compile_definitions(test_runner PRIVATE ENABLE_MOVE_ORDERING=$<BOOL:${ENABLE_MOVE_ORDERING}>)
target_include_directories(test_runner PRIVATE test/unity)
if(NOT MSVC)
    target_compile_options(test_runner PRIVATE -O1 -UNDEBUG)
else()
    # /UNDEBUG would conflict with /DNDEBUG from Release build flags, triggering D9025.
    # Use a force-included header to #undef NDEBUG at the preprocessor level instead.
    set(_undef_ndebug_h "${CMAKE_CURRENT_BINARY_DIR}/undef_ndebug.h")
    file(WRITE "${_undef_ndebug_h}" "#undef NDEBUG\n")
    target_compile_options(test_runner PRIVATE "/FI${_undef_ndebug_h}")
endif()

if(NOT MSVC)
    target_link_libraries(test_runner m)
endif()

if(ENABLE_NATIVE_OPTIMIZATIONS AND NOT MSVC)
    target_compile_options(test_runner PRIVATE -march=native)
endif()

enable_testing()
add_test(NAME unit_tests COMMAND test_runner)
