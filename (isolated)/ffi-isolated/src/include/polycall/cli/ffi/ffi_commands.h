/**
 * @file ffi_commands.h
 * @brief Command handlers for ffi module
 */

#ifndef POLYCALL_CLI_FFI_COMMANDS_H
#define POLYCALL_CLI_FFI_COMMANDS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Handle ffi commands
 *
 * @param argc Argument count
 * @param argv Argument vector
 * @param context Command context
 * @return int 0 on success, error code otherwise
 */
int ffi_command_handler(int argc, char** argv, void* context);

/**
 * Register ffi commands
 *
 * @return int 0 on success, error code otherwise
 */
int register_ffi_commands(void);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_CLI_FFI_COMMANDS_H */
