/**
 * @file polycall.h
 * @brief Main include file for LibPolyCall
 */

#ifndef POLYCALL_H
#define POLYCALL_H

/* Core includes */
#include "polycall/core/polycall/polycall.h"
#include "polycall/core/polycall/polycall_error.h"

/* CLI includes */
#include "polycall/cli/command.h"
#include "polycall/cli/repl.h"

/* Module-specific includes */
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

#endif /* POLYCALL_H */
