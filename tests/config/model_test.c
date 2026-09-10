/*
 * Unit checks for the schema-v2 typed model: builder -> validate -> canonical
 * envelope round-trip, and that invalid values fail with a stable code.
 * Linked against libpolycall (static). No runtime, no network.
 */
#include <stdio.h>
#include <string.h>
#include "polycall_config2.h"

static int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL  %s:%d  %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static polycall_config2_t *two_service(polycall_config2_error_t *err)
{
    polycall_config2_builder_t *b = polycall_config2_builder_create();
    int a, c;
    polycall_config2_t *cfg;
    polycall_config2_builder_set_project(b, "shop", "obinexus");

    a = polycall_config2_builder_add_service(b, "inventory", "c", err);
    polycall_config2_builder_service_set_ports(b, a, 8080, 9090, err);
    polycall_config2_builder_service_set_workspace(b, a, "/opt/shop/inventory", err);
    polycall_config2_builder_service_set_limits(b, a, 15000, 128, err);

    c = polycall_config2_builder_add_service(b, "web", "node", err);
    polycall_config2_builder_service_set_ports(b, c, 3000, 3001, err);
    polycall_config2_builder_service_set_workspace(b, c, "/opt/shop/web", err);

    cfg = polycall_config2_builder_build(b, err);
    polycall_config2_builder_free(b);
    return cfg;
}

int main(void)
{
    polycall_config2_error_t err;
    polycall_config2_t *cfg, *cfg2;
    char env1[4096], env2[4096];
    int n1;

    polycall_config2_error_init(&err);

    /* 1. build + canonical envelope */
    cfg = two_service(&err);
    CHECK(cfg != NULL);
    if (!cfg) { printf("build failed: %s %s\n", err.code, err.message); return 1; }
    CHECK(strcmp(polycall_config2_project_name(cfg), "shop") == 0);
    CHECK(polycall_config2_service_count(cfg) == 2);

    n1 = polycall_config2_to_envelope(cfg, env1, sizeof env1);
    CHECK(n1 > 0 && (size_t)n1 < sizeof env1);
    /* services must be in canonical (sorted) order: inventory before web */
    CHECK(strstr(env1, "\"id\":\"inventory\"") < strstr(env1, "\"id\":\"web\""));
    CHECK(strstr(env1, "\"schema_version\":2") != NULL);
    CHECK(strstr(env1, "\"host_port\":8080") != NULL);       /* number, not string */
    CHECK(strstr(env1, "\"tls\":{\"mode\":\"off\"") != NULL);

    /* 2. round-trip: parse the envelope, re-emit, expect byte-identical */
    cfg2 = polycall_config2_from_envelope(env1, strlen(env1), &err);
    CHECK(cfg2 != NULL);
    if (cfg2) {
        polycall_config2_to_envelope(cfg2, env2, sizeof env2);
        CHECK(strcmp(env1, env2) == 0);
        polycall_config2_free(cfg2);
    }

    /* 3. invalid port -> stable code */
    {
        polycall_config2_builder_t *b = polycall_config2_builder_create();
        int s;
        polycall_config2_builder_set_project(b, "p", NULL);
        s = polycall_config2_builder_add_service(b, "x", "c", &err);
        CHECK(polycall_config2_builder_service_set_ports(b, s, 70000, 10, &err) != 0);
        CHECK(strcmp(err.code, "port.range") == 0);
        polycall_config2_builder_free(b);
    }

    /* 4. duplicate service id */
    {
        polycall_config2_builder_t *b = polycall_config2_builder_create();
        polycall_config2_builder_set_project(b, "p", NULL);
        polycall_config2_builder_add_service(b, "dup", "c", &err);
        CHECK(polycall_config2_builder_add_service(b, "dup", "node", &err) < 0);
        CHECK(strcmp(err.code, "service.id.duplicate") == 0);
        polycall_config2_builder_free(b);
    }

    /* 5. unknown field in envelope */
    {
        const char *bad = "{\"schema_version\":2,\"project\":{\"name\":\"p\"},"
                          "\"services\":[{\"id\":\"a\",\"language\":\"c\","
                          "\"host_port\":1,\"target_port\":2,"
                          "\"workspace\":\"/w\",\"bogus\":1}]}";
        polycall_config2_t *c = polycall_config2_from_envelope(bad, strlen(bad), &err);
        CHECK(c == NULL);
        CHECK(strcmp(err.code, "field.unknown") == 0);
    }

    /* 6. tls enabled but inline secret instead of a reference */
    {
        polycall_config2_builder_t *b = polycall_config2_builder_create();
        int s;
        polycall_config2_builder_set_project(b, "p", NULL);
        s = polycall_config2_builder_add_service(b, "a", "c", &err);
        CHECK(polycall_config2_builder_service_set_tls(
                  b, s, POLYCALL_TLS_SERVER, "-----BEGIN CERT-----", "hunter2", &err) != 0);
        CHECK(strcmp(err.code, "tls.refs.invalid") == 0);
        polycall_config2_builder_free(b);
    }

    /* 7. port as a JSON string must be rejected (ports are numbers) */
    {
        const char *bad = "{\"schema_version\":2,\"project\":{\"name\":\"p\"},"
                          "\"services\":[{\"id\":\"a\",\"language\":\"c\","
                          "\"host_port\":\"8080\",\"target_port\":2,\"workspace\":\"/w\"}]}";
        polycall_config2_t *c = polycall_config2_from_envelope(bad, strlen(bad), &err);
        CHECK(c == NULL);
        CHECK(strncmp(err.code, "port", 4) == 0);
    }

    polycall_config2_free(cfg);

    if (fails == 0) {
        printf("PASS  config2 model: 7 groups OK\n");
        return 0;
    }
    printf("FAIL  config2 model: %d assertion(s)\n", fails);
    return 1;
}
