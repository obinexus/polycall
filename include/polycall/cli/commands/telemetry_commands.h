/**
 * @file telemetry_commands.h
 * @brief Telemetry command handlers for LibPolyCall CLI
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file defines the interface for telemetry-related command handlers for the LibPolyCall CLI.
 */

 #ifndef POLYCALL_CLI_TELEMETRY_COMMANDS_H
 #define POLYCALL_CLI_TELEMETRY_COMMANDS_H
 
 #include "polycall/cli/command.h"
#include "polycall/core/polycall/polycall_core.h"
 #include "polycall/core/protocol/polycall_protocol_context.h"
 #include "polycall/core/accessibility/accessibility_colors.h"
 #include "polycall/core/polycall/polycall_context.h"
 #include "polycall/core/polycall/polycall_state_machine.h"
 #include "polycall/core/protocol/polycall_protocol_context.h"

#include "polycall/cli/command.h"
 #include <stdbool.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /**
  * @brief Start telemetry command handler
  */
 command_result_t telemetry_cmd_start(int argc, char** argv, void* context);
 
 /**
  * @brief Stop telemetry command handler
  */
 command_result_t telemetry_cmd_stop(int argc, char** argv, void* context);
 
 /**
  * @brief Show telemetry status command handler
  */
 command_result_t telemetry_cmd_status(int argc, char** argv, void* context);
 
 /**
  * @brief List metrics command handler
  */
 command_result_t telemetry_cmd_list_metrics(int argc, char** argv, void* context);
 
 /**
  * @brief Get metric value command handler
  */
 command_result_t telemetry_cmd_get_metric(int argc, char** argv, void* context);
 
 /**
  * @brief Take metrics snapshot command handler
  */
 command_result_t telemetry_cmd_snapshot(int argc, char** argv, void* context);
 
 /**
  * @brief Generate telemetry report command handler
  */
 command_result_t telemetry_cmd_generate_report(int argc, char** argv, void* context);
 
 /**
  * @brief Export telemetry data command handler
  */
 command_result_t telemetry_cmd_export_data(int argc, char** argv, void* context);
 
 /**
  * @brief Get telemetry configuration command handler
  */
 command_result_t telemetry_cmd_get_config(int argc, char** argv, void* context);
 
 /**
  * @brief Set telemetry configuration command handler
  */
 command_result_t telemetry_cmd_set_config(int argc, char** argv, void* context);
 
 /**
  * @brief Add telemetry alert command handler
  */
 command_result_t telemetry_cmd_add_alert(int argc, char** argv, void* context);
 
 /**
  * @brief List telemetry alerts command handler
  */
 command_result_t telemetry_cmd_list_alerts(int argc, char** argv, void* context);
 
 /**
  * @brief Remove telemetry alert command handler
  */
 command_result_t telemetry_cmd_remove_alert(int argc, char** argv, void* context);
 
 /**
  * @brief Register telemetry commands
  */
 int telemetry_commands_register(void);
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* POLYCALL_CLI_TELEMETRY_COMMANDS_H */