#include <stdio.h>
#include <stdbool.h>

// === CONFIG ===
bool IS_SANDBOXED = true; // toggle this to test

// === NO-OP BASE ===
void noop() {
    // Does nothing, just a placeholder
}

// === POLICY LOGIC ===
#define POLIC_DECORATOR(func) ({ \
    void wrapped_##func() { \
        if (IS_SANDBOXED) { \
            printf("[POLIC] Sandbox policy active: blocking %s()\n", #func); \
            /* Here you can call noop(), log, or throw error */ \
        } else { \
            printf("[POLIC] Policy passed: executing %s()\n", #func); \
            func(); \
        } \
    } \
    wrapped_##func; \
})

// === TARGET FUNCTION ===
void send_net_data() {
    printf("Sending data over the network...\n");
}

// === MAIN ===
int main() {
    // Wrap the function in PoliC
    void (*secured_send)() = POLIC_DECORATOR(send_net_data);

    // Call the function — policy kicks in automatically
    secured_send();

    return 0;
}
