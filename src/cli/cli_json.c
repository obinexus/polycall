/*
 * Minimal JSON writer for finite `--format json` command results.
 *
 * Machine output is exactly one UTF-8 JSON object on stdout, for success and
 * failure alike. No banners, no ANSI. Logs and diagnostics go to stderr.
 */

#include "cli_internal.h"

#include <string.h>

char *polycall_json_quote(char *buf, size_t buf_len, const char *s)
{
    size_t w = 0;
    if (buf_len == 0) {
        return buf;
    }
    if (s == NULL) {
        /* caller asked for a quoted string of a NULL pointer: emit "" */
        s = "";
    }

    buf[w++] = '"';
    for (; *s && w + 7 < buf_len; ++s) {
        unsigned char c = (unsigned char)*s;
        switch (c) {
        case '"':  buf[w++] = '\\'; buf[w++] = '"';  break;
        case '\\': buf[w++] = '\\'; buf[w++] = '\\'; break;
        case '\b': buf[w++] = '\\'; buf[w++] = 'b';  break;
        case '\f': buf[w++] = '\\'; buf[w++] = 'f';  break;
        case '\n': buf[w++] = '\\'; buf[w++] = 'n';  break;
        case '\r': buf[w++] = '\\'; buf[w++] = 'r';  break;
        case '\t': buf[w++] = '\\'; buf[w++] = 't';  break;
        default:
            if (c < 0x20) {
                static const char hex[] = "0123456789abcdef";
                buf[w++] = '\\'; buf[w++] = 'u'; buf[w++] = '0'; buf[w++] = '0';
                buf[w++] = hex[(c >> 4) & 0xF];
                buf[w++] = hex[c & 0xF];
            } else {
                buf[w++] = (char)c; /* pass UTF-8 bytes through untouched */
            }
        }
    }
    if (w + 1 < buf_len) {
        buf[w++] = '"';
    }
    buf[w < buf_len ? w : buf_len - 1] = '\0';
    return buf;
}

void polycall_json_result(FILE *out, const char *command, bool ok,
                          const char *data_json,
                          const char *err_code, const char *err_msg,
                          const char *err_hint)
{
    char qcmd[256];
    char qcode[128];
    char qmsg[512];
    char qhint[512];

    fputs("{", out);
    fprintf(out, "\"schema_version\":%d", POLYCALL_CLI_SCHEMA_VERSION);
    fprintf(out, ",\"ok\":%s", ok ? "true" : "false");
    fprintf(out, ",\"command\":%s",
            polycall_json_quote(qcmd, sizeof qcmd, command ? command : ""));
    fprintf(out, ",\"data\":%s", (data_json && *data_json) ? data_json : "null");

    if (ok) {
        fputs(",\"error\":null", out);
    } else {
        fprintf(out, ",\"error\":{\"code\":%s",
                polycall_json_quote(qcode, sizeof qcode,
                                    err_code ? err_code : "error"));
        fprintf(out, ",\"message\":%s",
                polycall_json_quote(qmsg, sizeof qmsg,
                                    err_msg ? err_msg : "unspecified error"));
        if (err_hint && *err_hint) {
            fprintf(out, ",\"hint\":%s",
                    polycall_json_quote(qhint, sizeof qhint, err_hint));
        } else {
            fputs(",\"hint\":null", out);
        }
        fputs("}", out);
    }
    fputs("}\n", out);
}
