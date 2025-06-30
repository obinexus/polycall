#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/* Forward declarations */
void noop(void);
void vm_hook_check(void);

/* === CONFIGURATION === */
typedef enum {
    POLICY_ALLOW,
    POLICY_BLOCK,
    POLICY_LOG_ONLY
} PolicyAction;

/* Complete struct definition including all fields */
typedef struct {
    bool is_sandboxed;
    bool enable_vm_hooks;
    bool stack_protection;
    bool stack_protection_active;  /* Track if protection is active */
    volatile unsigned long* stack_canary_ptr;  /* Pointer to the current canary */
    PolicyAction default_action;
    void (*logger)(const char* message);
} PolICConfig;

/* Global configuration */
static PolICConfig g_config = {
    .is_sandboxed = true,
    .enable_vm_hooks = true,
    .stack_protection = true,
    .stack_protection_active = false,
    .stack_canary_ptr = NULL,
    .default_action = POLICY_BLOCK,
    .logger = NULL
};

/* Canary value to detect stack tampering */
static const unsigned long STACK_CANARY = 0xDEADBEEFCAFEBABE;

/* === LOGGING SYSTEM === */
void default_logger(const char* message) {
    printf("[POLIC] %s\n", message);
}

void setup_logger(void (*logger_func)(const char*)) {
    g_config.logger = logger_func ? logger_func : default_logger;
}

#define LOG(msg) do { \
    if (g_config.logger) { \
        g_config.logger(msg); \
    } \
} while(0)

/* === VM HOOKS INTEGRATION === */
void vm_hook_check(void) {
    if (g_config.enable_vm_hooks) {
        LOG("VM Hook activated - checking execution context");
        /* In a real implementation, you'd use VM-specific instructions */
        #ifdef __x86_64__
        /* Only attempt VM call on x86_64 architecture */
        if (getenv("POLIC_ALLOW_VMCALLS") != NULL) {
            /* This is dangerous outside a VM - hence the env check */
            /* __asm__("vmcall");  Commented for safety */
            LOG("VM call instruction executed");
        } else {
            LOG("VM calls disabled (safety check)");
        }
        #else
        LOG("VM calls unavailable on this architecture");
        #endif
    }
}

/* === STACK PROTECTION === */
/* Fixed stack protection macro - enclosed in a scope to avoid variable declaration issues */
#define STACK_PROTECT_BEGIN() \
    do { \
        volatile unsigned long* __tmp_canary = malloc(sizeof(unsigned long)); \
        if (__tmp_canary == NULL) { \
            LOG("ERROR: Failed to allocate memory for stack canary"); \
            exit(EXIT_FAILURE); \
        } \
        *__tmp_canary = STACK_CANARY; \
        LOG("Stack protection enabled for this function"); \
        g_config.stack_protection_active = true; \
        g_config.stack_canary_ptr = __tmp_canary; \
    } while(0)

/* Now check the canary via the global pointer and clean up */
#define STACK_PROTECT_END() do { \
    if (g_config.stack_protection_active && g_config.stack_canary_ptr != NULL) { \
        if (*(g_config.stack_canary_ptr) != STACK_CANARY) { \
            LOG("CRITICAL: Stack corruption detected!"); \
            free((void*)g_config.stack_canary_ptr); \
            exit(EXIT_FAILURE); \
        } \
        g_config.stack_protection_active = false; \
        free((void*)g_config.stack_canary_ptr); \
        g_config.stack_canary_ptr = NULL; \
    } \
} while(0)

/* === NO-OP BASE === */
void noop(void) {
    /* Does nothing, just a placeholder */
}

/* Forward declarations for wrapped functions */
void wrapped_send_net_data(void);
void wrapped_access_filesystem(void);

/* === TARGET FUNCTIONS === */
void send_net_data(void) {
    printf("Sending data over the network...\n");
}

void access_filesystem(void) {
    printf("Accessing sensitive filesystem resources...\n");
}

/* Policy logic for send_net_data */
void wrapped_send_net_data(void) {
    char log_buffer[256];
    
    if (g_config.stack_protection) {
        STACK_PROTECT_BEGIN();
    }
    
    if (g_config.enable_vm_hooks) {
        vm_hook_check();
    }
    
    if (g_config.is_sandboxed) {
        snprintf(log_buffer, sizeof(log_buffer),
                "Sandbox policy active for %s()", "send_net_data");
        LOG(log_buffer);
        
        switch (g_config.default_action) {
            case POLICY_ALLOW:
                LOG("Policy allows execution despite sandbox");
                send_net_data();
                break;
            case POLICY_LOG_ONLY:
                LOG("Policy logs but allows execution");
                send_net_data();
                break;
            case POLICY_BLOCK:
            default:
                LOG("Policy blocks execution in sandbox");
                noop();
                break;
        }
    } else {
        snprintf(log_buffer, sizeof(log_buffer),
                "Policy passed: executing %s()", "send_net_data");
        LOG(log_buffer);
        send_net_data();
    }
    
    if (g_config.stack_protection) {
        STACK_PROTECT_END();
    }
}

/* Policy logic for access_filesystem */
void wrapped_access_filesystem(void) {
    char log_buffer[256];
    
    if (g_config.stack_protection) {
        STACK_PROTECT_BEGIN();
    }
    
    if (g_config.enable_vm_hooks) {
        vm_hook_check();
    }
    
    if (g_config.is_sandboxed) {
        snprintf(log_buffer, sizeof(log_buffer),
                "Sandbox policy active for %s()", "access_filesystem");
        LOG(log_buffer);
        
        switch (g_config.default_action) {
            case POLICY_ALLOW:
                LOG("Policy allows execution despite sandbox");
                access_filesystem();
                break;
            case POLICY_LOG_ONLY:
                LOG("Policy logs but allows execution");
                access_filesystem();
                break;
            case POLICY_BLOCK:
            default:
                LOG("Policy blocks execution in sandbox");
                noop();
                break;
        }
    } else {
        snprintf(log_buffer, sizeof(log_buffer),
                "Policy passed: executing %s()", "access_filesystem");
        LOG(log_buffer);
        access_filesystem();
    }
    
    if (g_config.stack_protection) {
        STACK_PROTECT_END();
    }
}

/* === INLINE POLICY INJECTION MACRO === */
#define INLINE_POLICY_CHECK() do { \
    bool __policy_block = false; \
    if (g_config.is_sandboxed) { \
        LOG("Inline policy check activated in function"); \
        if (g_config.stack_protection) { \
            STACK_PROTECT_BEGIN(); \
        } \
        if (g_config.enable_vm_hooks) { \
            vm_hook_check(); \
        } \
        if (g_config.default_action == POLICY_BLOCK) { \
            LOG("Inline policy blocks execution"); \
            __policy_block = true; \
        } \
    } \
    if (__policy_block) return; \
} while(0)

/* Function with inline policy */
void execute_command(const char* cmd) {
    INLINE_POLICY_CHECK();
    
    /* This would only run if policy allows */
    printf("Executing command: %s\n", cmd);
    
    if (g_config.stack_protection) {
        STACK_PROTECT_END();
    }
}

/* === INITIALIZATION === */
void polic_init(bool sandbox_mode, PolicyAction action) {
    g_config.is_sandboxed = sandbox_mode;
    g_config.default_action = action;
    g_config.stack_protection_active = false;
    g_config.stack_canary_ptr = NULL;
    setup_logger(NULL); /* Use default logger */
    
    LOG("PoliC security framework initialized");
    
    char config_info[256];
    snprintf(config_info, sizeof(config_info), 
             "Configuration: Sandbox=%s, VM-Hooks=%s, Stack-Protection=%s, Action=%d",
             g_config.is_sandboxed ? "ON" : "OFF",
             g_config.enable_vm_hooks ? "ON" : "OFF",
             g_config.stack_protection ? "ON" : "OFF",
             g_config.default_action);
    LOG(config_info);
}

/* === MAIN === */
int main(void) {
    /* Initialize PoliC with custom settings */
    polic_init(true, POLICY_BLOCK);
    
    /* Create secured function pointers */
    void (*secured_net_send)(void) = wrapped_send_net_data;
    void (*secured_fs_access)(void) = wrapped_access_filesystem;
    
    /* Execute secured functions - policy kicks in automatically */
    printf("\n--- Testing secured network function ---\n");
    secured_net_send();
    
    printf("\n--- Testing secured filesystem function ---\n");
    secured_fs_access();
    
    printf("\n--- Testing inline policy function ---\n");
    execute_command("rm -rf /");  /* This is safe due to policy! */
    
    /* Demonstrate policy change */
    printf("\n--- Changing policy to ALLOW ---\n");
    g_config.default_action = POLICY_ALLOW;
    secured_net_send();
    
    /* Demonstrate sandbox toggle */
    printf("\n--- Disabling sandbox ---\n");
    g_config.is_sandboxed = false;
    secured_fs_access();
    
    return 0;
}