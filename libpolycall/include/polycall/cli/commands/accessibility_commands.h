/**
 * @file accessibility_commands.h
 * @brief Command handlers for accessibility module
 */

#ifndef POLYCALL_CLI_ACCESSIBILITY_COMMANDS_H
#define POLYCALL_CLI_ACCESSIBILITY_COMMANDS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Handle accessibility commands
 *
 * @param argc Argument count
 * @param argv Argument vector
 * @param context Command context
 * @return int 0 on success, error code otherwise
 */
int accessibility_command_handler(int argc, char** argv, void* context);

/**
 * Register accessibility commands
 *
 * @return int 0 on success, error code otherwise
 */
int register_accessibility_commands(void);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_CLI_ACCESSIBILITY_COMMANDS_H */
