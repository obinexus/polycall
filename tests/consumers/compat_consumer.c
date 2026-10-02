/* Existing consumers that still include the deprecated forwarding header
 * keep compiling (with a deprecation notice) and keep working. */
#include <libpolycall.h>
#include <stdio.h>

int main(void)
{
    printf("%s\n", polycall_get_version());
    return 0;
}
