#include "proto_stub.h"
#include <string.h>

int polycall_stub_authorize(const char* agent_id, const char* endpoint) {
    if (!agent_id || !endpoint) return -1;

    if (strcmp(agent_id, "demo-agent") == 0 &&
        strcmp(endpoint, "/api/secure") == 0) {
        return 1; // Allow
    }

    return 0; // Deny by default (zero trust)
}
