/**
 * @file config_commands.h
 * @brief Command handlers for config module
 */

#ifndef POLYCALL_CLI_CONFIG_COMMANDS_H
#define POLYCALL_CLI_CONFIG_COMMANDS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Handle config commands
 *
 * @param argc Argument count
 * @param argv Argument vector
 * @param context Command context
 * @return int 0 on success, error code otherwise
 */
int config_command_handler(int argc, char** argv, void* context);

/**
 * Register config commands
 *
 * @return int 0 on success, error code otherwise
 */
int register_config_commands(void);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_CLI_CONFIG_COMMANDS_H */
