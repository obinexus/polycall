/**
 * @file test_core.c
 * @brief Unit tests for core PoliC functionality
 */

#include <polic/polic.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

void test_init() {
    int result = polic_init(true, POLIC_BLOCK);
    assert(result == 0);
    printf("init test passed\n");
}

int main() {
    test_init();
    printf("All tests passed\n");
    return 0;
}
