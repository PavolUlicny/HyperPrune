#include "unity/unity.h"
#include <stdio.h>

// Forward declarations from test files
void test_bitboard_suite(void);
void test_minimax_suite(void);
void test_zobrist_suite(void);
void test_correctness_suite(void);
void test_transposition_table_suite(void);
void test_game_scenarios_suite(void);
void test_edge_cases_suite(void);

void setUp(void) {
    // Called before each test
}

void tearDown(void) {
    // Called after each test
}

int main(void) {
    UNITY_BEGIN();

    printf("\n=== Bitboard Tests ===\n");
    test_bitboard_suite();

    printf("\n=== Minimax Tests ===\n");
    test_minimax_suite();

    printf("\n=== Zobrist Hashing Tests ===\n");
    test_zobrist_suite();

    printf("\n=== Transposition Table Tests ===\n");
    test_transposition_table_suite();

    printf("\n=== Game Scenarios Tests ===\n");
    test_game_scenarios_suite();

    printf("\n=== Edge Cases Tests ===\n");
    test_edge_cases_suite();

    printf("\n=== Correctness Tests ===\n");
    test_correctness_suite();

    return UNITY_END();
}
