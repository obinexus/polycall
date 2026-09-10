/*
 * Example C configuration provider (schema v2).
 *
 * Built as a shared library with the normal native toolchain and loaded by
 * `polycall config validate --provider c:<path>` through
 * polycall_provider_load_c(). It expresses the same two-service "shop"
 * topology as the Node and Python example providers; the three must yield a
 * byte-identical canonical envelope.
 *
 * This is real C: values are C literals fed to the versioned builder API.
 */

#include "polycall_config2.h"
#include "polycall_provider.h"

#if defined(_WIN32)
#  define PROVIDER_EXPORT __declspec(dllexport)
#else
#  define PROVIDER_EXPORT __attribute__((visibility("default")))
#endif

PROVIDER_EXPORT int POLYCALL_CALL
polycall_config_provider_v1(polycall_config2_builder_t *b,
                            const polycall_provider_ctx_t *ctx)
{
    polycall_config2_error_t err;
    int inv, web;

    polycall_config2_error_init(&err);
    (void)ctx; /* base_dir / project_root / language available if needed */

    if (polycall_config2_builder_set_project(b, "shop", "obinexus") != 0) {
        return 1;
    }

    inv = polycall_config2_builder_add_service(b, "inventory", "c", &err);
    if (inv < 0) return 2;
    polycall_config2_builder_service_set_ports(b, inv, 8080, 9090, &err);
    polycall_config2_builder_service_set_bind(b, inv, "127.0.0.1", &err);
    polycall_config2_builder_service_set_workspace(b, inv, "/opt/shop/inventory", &err);
    polycall_config2_builder_service_set_limits(b, inv, 15000, 128, &err);

    web = polycall_config2_builder_add_service(b, "web", "node", &err);
    if (web < 0) return 3;
    polycall_config2_builder_service_set_ports(b, web, 3000, 3001, &err);
    polycall_config2_builder_service_set_bind(b, web, "0.0.0.0", &err);
    polycall_config2_builder_service_set_workspace(b, web, "/opt/shop/web", &err);
    polycall_config2_builder_service_set_limits(b, web, 30000, 64, &err);

    return 0;
}
