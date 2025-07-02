/* Standard library includes */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>

/* Core types */
#include "polycall/core/types.h"

/**
 * @file polycall_commands.c
 * @brief Main command system for LibPolyCall CLI with IoC integration
 */

#include "polycall/cli/common/command_registry.h"
#include "polycall/cli/command.h"
#include "polycall/core/polycall/polycall.h"

// Include all module command registration headers
#include "polycall/cli/accessibility/accessibility_commands.h"
#include "polycall/cli/auth/auth_commands.h"
#include "polycall/cli/config/config_commands.h"
#include "polycall/cli/edge/edge_commands.h"
#include "polycall/cli/ffi/ffi_commands.h"
#include "polycall/cli/micro/micro_commands.h"
#include "polycall/cli/network/network_commands.h"
#include "polycall/cli/protocol/protocol_commands.h"
#include "polycall/cli/repl/repl_commands.h"
#include "polycall/cli/telemetry/telemetry_commands.h"

/**
 * Register all command modules
 */
bool register_all_commands(void) {
    // Register each module's commands
    if (!register_accessibility_commands()) {
        return false;
    }
    
    if (!register_auth_commands()) {
        return false;
    }
    
    if (!register_config_commands()) {
        return false;
    }
    
    if (!register_edge_commands()) {
        return false;
    }
    
    if (!register_ffi_commands()) {
        return false;
    }
    
    if (!register_micro_commands()) {
        return false;
    }
    
    if (!register_network_commands()) {
        return false;
    }
    
    if (!register_protocol_commands()) {
        return false;
    }
    
    if (!register_repl_commands()) {
        return false;
    }
    
    if (!register_telemetry_commands()) {
        return false;
    }

    return true;
}
