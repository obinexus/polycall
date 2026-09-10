/*
 * Consumer #2 - import-library / shared linkage.
 * Compiled with POLYCALL_USE_SHARED so the ABI is declared dllimport on
 * Windows and default-visibility elsewhere. Linked against the import lib
 * (libpolycall.dll.a) on Windows or -lpolycall on ELF/Mach-O.
 */
#define POLYCALL_USE_SHARED
#include <stdio.h>
#include "polycall.h"

int main(void)
{
    const char *v = polycall_get_version();
    if (!v || !*v) {
        fprintf(stderr, "link_shared_consumer: empty version\n");
        return 2;
    }
    printf("%s\n", v);
    return 0;
}
