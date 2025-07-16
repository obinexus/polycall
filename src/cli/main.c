#include <stdio.h>
#include <stdlib.h>

extern int polycall_init(void);
extern int polycall_cleanup(void);

int main(int argc, char *argv[]) {
    printf("Polycall CLI v2.0.0\n");
    
    if (polycall_init() != 0) {
        fprintf(stderr, "Failed to initialize polycall\n");
        return 1;
    }
    
    printf("Polycall initialized successfully\n");
    
    if (argc > 1) {
        printf("Arguments: ");
        for (int i = 1; i < argc; i++) {
            printf("%s ", argv[i]);
        }
        printf("\n");
    }
    
    polycall_cleanup();
    return 0;
}
