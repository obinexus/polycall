/**
 * @file polycall_commands.h
 * @brief Main command system interface for LibPolyCall CLI
 */

#ifndef POLYCALL_CLI_COMMANDS_H
#define POLYCALL_CLI_COMMANDS_H

#include <stdbool.h>

/**
 * Register all command modules
 *
 * @return true if all commands registered successfully, false otherwise
 */
bool register_all_commands(void);

#endif /* POLYCALL_CLI_COMMANDS_H */
