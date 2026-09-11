/*
 * Fixture operation plugin for `polycall run --load`.
 *
 * Exports polycall_ops_register (see include/polycall_runtime.h) and adds one
 * deterministic operation, demo.greet, reachable from the C CLI and every
 * language client with no core rebuild.
 *
 * Build this file twice to exercise the ABI-mismatch path without a second
 * source file: the normal build (FIXTURE_ABI_MAJOR unset -> matches the
 * runtime) and a "bad ABI" build with -DFIXTURE_ABI_MAJOR=999.
 */

#include "polycall_runtime.h"

#include <stdio.h>
#include <string.h>

#if defined(_WIN32)
#  define PLUGIN_EXPORT __declspec(dllexport)
#else
#  define PLUGIN_EXPORT __attribute__((visibility("default")))
#endif

#ifndef FIXTURE_ABI_MAJOR
#define FIXTURE_ABI_MAJOR POLYCALL_RUNTIME_ABI_VERSION
#endif

static polycall_op_status_t op_demo_greet(
    const char *input_json, uint32_t deadline_ms,
    char *out, size_t out_cap, char *ec, size_t ec_cap,
    char *em, size_t em_cap, void *user)
{
    /* A tiny hand-rolled string pull: input is always a compact
     * {"name":"..."} object built by the runtime's own JSON re-emit, so a
     * direct scan avoids pulling in a JSON parser from a fixture plugin. */
    const char *key = "\"name\":\"";
    const char *p = strstr(input_json, key);
    char name[128];
    size_t i = 0;
    (void)deadline_ms; (void)user; (void)ec; (void)ec_cap;

    if (!p) {
        snprintf(ec, ec_cap, "input.invalid");
        snprintf(em, em_cap, "demo.greet requires a string 'name'");
        return POLYCALL_OP_ERR_INPUT;
    }
    p += strlen(key);
    while (*p && *p != '"' && i + 1 < sizeof name) {
        name[i++] = *p++;
    }
    name[i] = '\0';
    if (i == 0) {
        snprintf(ec, ec_cap, "input.invalid");
        snprintf(em, em_cap, "demo.greet requires a non-empty 'name'");
        return POLYCALL_OP_ERR_INPUT;
    }

    snprintf(out, out_cap, "{\"message\":\"hello, %s\"}", name);
    return POLYCALL_OP_OK;
}

PLUGIN_EXPORT int POLYCALL_CALL
polycall_ops_register(polycall_runtime_t *rt, uint32_t abi_major)
{
    static const polycall_op_desc_t desc = {
        "demo", "greet", "fixture: greet a name (loaded, not built in)",
        "{\"name\":\"string\"}", "{\"message\":\"string\"}",
        true, op_demo_greet, NULL
    };

    if (abi_major != (uint32_t)FIXTURE_ABI_MAJOR) {
        return POLYCALL_PLUGIN_ABI_MISMATCH;
    }
    if (polycall_runtime_register(rt, &desc) != 0) {
        return POLYCALL_PLUGIN_ERROR;   /* e.g. a duplicate demo.greet */
    }
    return POLYCALL_PLUGIN_OK;
}
