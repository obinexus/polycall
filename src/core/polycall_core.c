#include <stdio.h>
#include <stdlib.h>

#ifndef POLYCALL_VERSION
#define POLYCALL_VERSION "2.0.0"
#endif

int polycall_init(void) {
    printf("Polycall Core v%s initialized\n", POLYCALL_VERSION);
    return 0;
}

int polycall_cleanup(void) {
    printf("Polycall Core cleanup\n");
    return 0;
}
