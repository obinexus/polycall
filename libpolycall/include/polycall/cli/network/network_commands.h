/**
 * @file network_commands.h
 * @brief Command handlers for network module
 */

#ifndef POLYCALL_CLI_NETWORK_COMMANDS_H
#define POLYCALL_CLI_NETWORK_COMMANDS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Handle network commands
 *
 * @param argc Argument count
 * @param argv Argument vector
 * @param context Command context
 * @return int 0 on success, error code otherwise
 */
int network_command_handler(int argc, char** argv, void* context);

/**
 * Register network commands
 *
 * @return int 0 on success, error code otherwise
 */
int register_network_commands(void);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_CLI_NETWORK_COMMANDS_H */
