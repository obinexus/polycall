#ifndef POLYCALL_CLI_H
#define POLYCALL_CLI_H

/*
 * PolyCall command-line entry point (schema v1).
 *
 * This is a separately versioned surface from the core library ABI. It is
 * provided by the `polycall` program (src/cli + src/main.c), not by
 * libpolycall; help/version/doctor never touch runtime initialisation, the
 * network, or a configuration file.
 */

#include "polycall_export.h"

POLYCALL_BEGIN_DECLS

#define POLYCALL_CLI_SCHEMA_VERSION 1

/*
 * Process exit codes. These are the shell contract: library status enums are
 * mapped onto these centrally and never leaked directly.
 */
typedef enum {
    POLYCALL_EXIT_OK              = 0,  /* completed successfully                */
    POLYCALL_EXIT_RUNTIME         = 1,  /* runtime / internal failure           */
    POLYCALL_EXIT_USAGE           = 2,  /* CLI usage error                      */
    POLYCALL_EXIT_CONFIG          = 3,  /* invalid or missing selected config   */
    POLYCALL_EXIT_UNSUPPORTED     = 4,  /* missing dependency / capability      */
    POLYCALL_EXIT_TRANSPORT       = 5,  /* transport / endpoint unavailable     */
    POLYCALL_EXIT_DEADLINE        = 6,  /* operation deadline exceeded          */
    POLYCALL_EXIT_AUTH            = 7,  /* authn / authz rejected               */
    POLYCALL_EXIT_INTERRUPT       = 130 /* user interruption (SIGINT)           */
} polycall_exit_t;

/**
 * Run the PolyCall CLI.
 *
 * @param argc  process argument count (argv[0] is the program name)
 * @param argv  process argument vector, OS-provided boundaries preserved
 * @return a polycall_exit_t value
 */
int polycall_cli_main(int argc, char **argv);

POLYCALL_END_DECLS

#endif /* POLYCALL_CLI_H */
