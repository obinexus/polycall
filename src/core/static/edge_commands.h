/**
 * @file edge_commands.h
 * @brief Command handlers for edge module
 */

#ifndef POLYCALL_CLI_EDGE_COMMANDS_H
#define POLYCALL_CLI_EDGE_COMMANDS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Handle edge commands
 *
 * @param argc Argument count
 * @param argv Argument vector
 * @param context Command context
 * @return int 0 on success, error code otherwise
 */
int edge_command_handler(int argc, char** argv, void* context);

/**
 * Register edge commands
 *
 * @return int 0 on success, error code otherwise
 */
int register_edge_commands(void);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_CLI_EDGE_COMMANDS_H */
