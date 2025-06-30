/**
 * @file telemetry_commands.h
 * @brief Command handlers for telemetry module
 */

#ifndef POLYCALL_CLI_TELEMETRY_COMMANDS_H
#define POLYCALL_CLI_TELEMETRY_COMMANDS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Handle telemetry commands
 *
 * @param argc Argument count
 * @param argv Argument vector
 * @param context Command context
 * @return int 0 on success, error code otherwise
 */
int telemetry_command_handler(int argc, char** argv, void* context);

/**
 * Register telemetry commands
 *
 * @return int 0 on success, error code otherwise
 */
int register_telemetry_commands(void);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_CLI_TELEMETRY_COMMANDS_H */
