/**
 * @file protocol_commands.h
 * @brief Command handlers for protocol module
 */

#ifndef POLYCALL_CLI_PROTOCOL_COMMANDS_H
#define POLYCALL_CLI_PROTOCOL_COMMANDS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Handle protocol commands
 *
 * @param argc Argument count
 * @param argv Argument vector
 * @param context Command context
 * @return int 0 on success, error code otherwise
 */
int protocol_command_handler(int argc, char **argv, void *context);

/**
 * Register protocol commands
 *
 * @return int 0 on success, error code otherwise
 */
int register_protocol_commands(void);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_CLI_PROTOCOL_COMMANDS_H */
