/*
 * Consumer #1 - static linkage.
 * Links libpolycall.a directly. Exercises the preserved public ABI:
 * polycall_get_version / polycall_init_with_config / polycall_cleanup.
 */
#include <stdio.h>
#include <string.h>
#include "polycall.h"

int main(void)
{
    polycall_context_t ctx = NULL;
    polycall_config_t cfg;
    const char *v = polycall_get_version();

    if (!v || !*v) {
        fprintf(stderr, "static_consumer: empty version\n");
        return 2;
    }
    printf("%s\n", v);

    memset(&cfg, 0, sizeof cfg);
    cfg.memory_pool_size = 128 * 1024;
    if (polycall_init_with_config(&ctx, &cfg) != POLYCALL_SUCCESS || ctx == NULL) {
        fprintf(stderr, "static_consumer: init failed\n");
        return 3;
    }
    polycall_cleanup(ctx);
    return 0;
}
