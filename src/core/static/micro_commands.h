/**
 * @file micro_commands.h
 * @brief Command handlers for micro module
 */

#ifndef POLYCALL_CLI_MICRO_COMMANDS_H
#define POLYCALL_CLI_MICRO_COMMANDS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Handle micro commands
 *
 * @param argc Argument count
 * @param argv Argument vector
 * @param context Command context
 * @return int 0 on success, error code otherwise
 */
int micro_command_handler(int argc, char **argv, void *context);

/**
 * Register micro commands
 *
 * @return int 0 on success, error code otherwise
 */
int register_micro_commands(void);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_CLI_MICRO_COMMANDS_H */
