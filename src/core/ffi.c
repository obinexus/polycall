/*
 * Binding ABI v1: configuration and RPC entry points (see polycall.h).
 * The peer-node entry points live in src/peer/peer.c.
 */

#include "polycall.h"

#include "pc_buf.h"
#include "pc_sys.h"
#include "status.h"
#include "../config/cfgfile.h"
#include "../config/json.h"
#include "../runtime/rpc_wire.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int map_cfg_status(int st)
{
    switch (st) {
    case POLYCALL_CFGFILE_OK:               return POLYCALL_OK;
    case POLYCALL_CFGFILE_INVALID_ARGUMENT: return POLYCALL_E_INVALID_ARGUMENT;
    case POLYCALL_CFGFILE_NOT_FOUND:        return POLYCALL_E_NOT_FOUND;
    case POLYCALL_CFGFILE_NO_MEMORY:        return POLYCALL_E_NO_MEMORY;
    default:                                return POLYCALL_E_CONFIG;
    }
}

int polycall_ffi_run_config(const char *config_path, int run)
{
    char err[PC_ERR_MAX];
    int st = 0;
    polycall_cfgfile_t *cf;
    const char *tls;

    pc_err_clear();
    if (!config_path || !*config_path) {
        return pc_err(POLYCALL_E_INVALID_ARGUMENT, "config_path is NULL or empty");
    }
    cf = polycall_cfgfile_load(config_path,
                               POLYCALL_CFGFILE_LEGACY_REQUIRED |
                               (run ? POLYCALL_CFGFILE_STRICT : 0u),
                               &st, err, sizeof err);
    if (!cf) {
        return pc_err(map_cfg_status(st), "%s", err[0] ? err : "invalid configuration");
    }
    tls = polycall_cfgfile_get(cf, "tls_enabled");
    if (run && tls && strcmp(tls, "true") == 0) {
        polycall_cfgfile_free(cf);
        return pc_err(POLYCALL_E_UNSUPPORTED,
                      "%s: tls_enabled=true, but this build has no TLS transport; "
                      "refusing rather than serving plaintext", config_path);
    }
    polycall_cfgfile_free(cf);
    return POLYCALL_OK;
}

int polycall_ffi_describe(const char *config_path, char *buf, int len)
{
    char err[PC_ERR_MAX];
    int st = 0;
    polycall_cfgfile_t *cf;
    pc_buf_t b;
    size_t n;

    pc_err_clear();
    if (len < 0 || (len > 0 && !buf)) {
        return pc_err(POLYCALL_E_INVALID_ARGUMENT, "invalid output buffer");
    }
    if (!config_path || !*config_path) {
        return pc_err(POLYCALL_E_INVALID_ARGUMENT, "config_path is NULL or empty");
    }
    cf = polycall_cfgfile_load(config_path, 0u, &st, err, sizeof err);
    if (!cf) {
        return pc_err(map_cfg_status(st), "%s", err[0] ? err : "invalid configuration");
    }
    pc_buf_init(&b, 1u << 20);
    if (polycall_cfgfile_describe(cf, &b) != 0) {
        polycall_cfgfile_free(cf);
        pc_buf_free(&b);
        return pc_err(POLYCALL_E_NO_MEMORY, "out of memory");
    }
    polycall_cfgfile_free(cf);
    n = b.len;
    if (n > (size_t)0x7FFFFFFF) {
        pc_buf_free(&b);
        return pc_err(POLYCALL_E_TOO_LARGE, "description too large");
    }
    pc_copy_out(buf, (size_t)len, b.data);
    pc_buf_free(&b);
    /* snprintf semantics: the return value is the full length */
    return (int)n;
}

int polycall_call(const char *endpoint, const char *service, const char *operation,
                  const char *input_json, uint32_t timeout_ms,
                  char *out, size_t out_cap, size_t *out_len)
{
    char host[POLYCALL_ENDPOINT_MAX], jerr[64];
    uint16_t port = 0;
    pc_buf_t req, res;
    pcr_frame_t rep;
    json_value *in = NULL, *r = NULL;
    const json_value *payload_value;
    int rc, status;
    bool ok = false;

    pc_err_clear();
    if (out_len) *out_len = 0;
    if (out && out_cap) out[0] = '\0';
    if (out_cap && !out) return pc_err(POLYCALL_E_INVALID_ARGUMENT, "out is NULL but out_cap > 0");
    if (!endpoint || pc_split_endpoint(endpoint, host, sizeof host, &port, 0) != POLYCALL_OK) {
        return pc_err(POLYCALL_E_INVALID_ARGUMENT, "endpoint must be host:port");
    }
    if (!service || !*service || !operation || !*operation) {
        return pc_err(POLYCALL_E_INVALID_ARGUMENT, "service and operation are required");
    }
    if (timeout_ms == 0 || timeout_ms > 600000u) {
        return pc_err(POLYCALL_E_INVALID_ARGUMENT, "timeout_ms must be 1..600000");
    }
    if (input_json) {
        in = json_parse(input_json, strlen(input_json), jerr, sizeof jerr);
        if (!in) return pc_err(POLYCALL_E_INVALID_ARGUMENT, "input_json is not valid JSON: %s", jerr);
    }

    pc_buf_init(&req, PCR_MAX_PAYLOAD);
    pc_buf_append(&req, "{\"service\":");
    pc_buf_json_string(&req, service);
    pc_buf_append(&req, ",\"operation\":");
    pc_buf_json_string(&req, operation);
    pc_buf_appendf(&req, ",\"deadline_ms\":%u,\"input\":", (unsigned)timeout_ms);
    pc_buf_json_value(&req, in);
    pc_buf_append(&req, "}");
    json_free(in);
    if (pc_buf_failed(&req)) {
        pc_buf_free(&req);
        return pc_err(POLYCALL_E_TOO_LARGE, "request exceeds the 1 MiB frame limit");
    }

    /* the server enforces timeout_ms on the operation; allow transport slack */
    rc = pcr_roundtrip(host, port, PCR_T_REQUEST, req.data, (uint32_t)req.len, &rep,
                       timeout_ms > 600000u - 2000u ? 600000u : timeout_ms + 2000u);
    pc_buf_free(&req);
    if (rc == -3) return pc_err(POLYCALL_E_TIMEOUT, "no reply from %s within the deadline", endpoint);
    if (rc == -2) return pc_err(POLYCALL_E_PROTOCOL, "%s sent a malformed frame", endpoint);
    if (rc != 0) return pc_err(POLYCALL_E_TRANSPORT, "cannot reach a polycall runtime at %s", endpoint);

    r = json_parse(rep.payload, rep.length, jerr, sizeof jerr);
    pcr_frame_free(&rep);
    if (!r || r->type != JSON_OBJECT) {
        json_free(r);
        return pc_err(POLYCALL_E_PROTOCOL, "reply from %s is not a JSON object", endpoint);
    }
    ok = json_bool(json_get(r, "ok"), false, NULL);
    payload_value = ok ? json_get(r, "output") : json_get(r, "error");

    pc_buf_init(&res, POLYCALL_CALL_MAX_OUTPUT);
    pc_buf_json_value(&res, payload_value);
    if (pc_buf_failed(&res)) {
        json_free(r);
        pc_buf_free(&res);
        return pc_err(POLYCALL_E_TOO_LARGE, "reply output exceeds the limit");
    }
    if (ok) {
        status = POLYCALL_OK;
    } else {
        const char *code = json_str(json_get(json_get(r, "error"), "code"), "", NULL);
        const char *msg = json_str(json_get(json_get(r, "error"), "message"), "", NULL);
        if (!strcmp(code, "server.busy")) status = POLYCALL_E_BUSY;
        else if (!strcmp(code, "operation.unknown")) status = POLYCALL_E_NOT_FOUND;
        else if (!strcmp(code, "deadline.exceeded")) status = POLYCALL_E_TIMEOUT;
        else if (!strcmp(code, "auth.denied")) status = POLYCALL_E_AUTH;
        else status = POLYCALL_E_REMOTE;
        pc_err(status, "%s.%s failed remotely: %s (%s)", service, operation,
               msg[0] ? msg : "error", code[0] ? code : "unknown");
    }
    json_free(r);

    if (out_len) *out_len = res.len;
    if (!out || out_cap <= res.len) {
        size_t need = res.len;
        pc_buf_free(&res);
        if (out && out_cap) out[0] = '\0';
        return pc_err(POLYCALL_E_TOO_LARGE,
                      "output buffer too small: need %lu bytes plus NUL "
                      "(the operation already ran)", (unsigned long)need);
    }
    memcpy(out, res.data, res.len + 1);
    pc_buf_free(&res);
    return status;
}
