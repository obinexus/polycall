/* An older consumer that still includes the deprecated header. */
#include <libpolycall.h>
#include <stdio.h>

int main(void)
{
    printf("%s\n", polycall_get_version());
    return 0;
}
