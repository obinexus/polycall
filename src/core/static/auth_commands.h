/**
 * @file auth_commands.h
 * @brief Command handlers for auth module
 */

#ifndef POLYCALL_CLI_AUTH_COMMANDS_H
#define POLYCALL_CLI_AUTH_COMMANDS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Handle auth commands
 *
 * @param argc Argument count
 * @param argv Argument vector
 * @param context Command context
 * @return int 0 on success, error code otherwise
 */
int auth_command_handler(int argc, char **argv, void *context);

/**
 * Register auth commands
 *
 * @return int 0 on success, error code otherwise
 */
int register_auth_commands(void);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_CLI_AUTH_COMMANDS_H */
