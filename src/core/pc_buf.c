#include "pc_buf.h"

#include "../config/json.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void pc_buf_init(pc_buf_t *b, size_t limit)
{
    b->data = NULL;
    b->len = 0;
    b->cap = 0;
    b->limit = limit;
    b->failed = false;
}

void pc_buf_free(pc_buf_t *b)
{
    if (!b) return;
    free(b->data);
    b->data = NULL;
    b->len = b->cap = 0;
    b->failed = false;
}

void pc_buf_reset(pc_buf_t *b)
{
    b->len = 0;
    b->failed = false;
    if (b->data) b->data[0] = '\0';
}

bool pc_buf_failed(const pc_buf_t *b)
{
    return b->failed;
}

char *pc_buf_take(pc_buf_t *b, size_t *len)
{
    char *d = b->data;
    if (len) *len = b->len;
    if (!d) {
        d = malloc(1);
        if (d) d[0] = '\0';
    }
    b->data = NULL;
    b->len = b->cap = 0;
    b->failed = false;
    return d;
}

static bool reserve(pc_buf_t *b, size_t extra)
{
    size_t need, ncap;
    char *nd;
    if (b->failed) return false;
    if (extra > SIZE_MAX - b->len - 1) { b->failed = true; return false; }
    need = b->len + extra + 1;
    if (b->limit && b->len + extra > b->limit) { b->failed = true; return false; }
    if (need <= b->cap) return true;
    ncap = b->cap ? b->cap : 64;
    while (ncap < need) {
        if (ncap > SIZE_MAX / 2) { ncap = need; break; }
        ncap *= 2;
    }
    nd = realloc(b->data, ncap);
    if (!nd) { b->failed = true; return false; }
    b->data = nd;
    b->cap = ncap;
    return true;
}

bool pc_buf_append_n(pc_buf_t *b, const void *data, size_t n)
{
    if (!reserve(b, n)) return false;
    if (n) memcpy(b->data + b->len, data, n);
    b->len += n;
    b->data[b->len] = '\0';
    return true;
}

bool pc_buf_append(pc_buf_t *b, const char *s)
{
    return pc_buf_append_n(b, s ? s : "", s ? strlen(s) : 0);
}

bool pc_buf_vappendf(pc_buf_t *b, const char *fmt, va_list ap)
{
    va_list cp;
    int n;
    if (b->failed) return false;
    va_copy(cp, ap);
    n = vsnprintf(NULL, 0, fmt, cp);
    va_end(cp);
    if (n < 0) { b->failed = true; return false; }
    if (!reserve(b, (size_t)n)) return false;
    vsnprintf(b->data + b->len, (size_t)n + 1, fmt, ap);
    b->len += (size_t)n;
    return true;
}

bool pc_buf_appendf(pc_buf_t *b, const char *fmt, ...)
{
    va_list ap;
    bool ok;
    va_start(ap, fmt);
    ok = pc_buf_vappendf(b, fmt, ap);
    va_end(ap);
    return ok;
}

/* length of a valid UTF-8 sequence starting at s (<= n bytes), or 0 */
static size_t utf8_seq(const unsigned char *s, size_t n)
{
    unsigned char c = s[0];
    size_t len, i;
    uint32_t cp;
    if (c < 0x80) return 1;
    if ((c & 0xE0) == 0xC0) { len = 2; cp = c & 0x1F; }
    else if ((c & 0xF0) == 0xE0) { len = 3; cp = c & 0x0F; }
    else if ((c & 0xF8) == 0xF0) { len = 4; cp = c & 0x07; }
    else return 0;
    if (len > n) return 0;
    for (i = 1; i < len; ++i) {
        if ((s[i] & 0xC0) != 0x80) return 0;
        cp = (cp << 6) | (s[i] & 0x3F);
    }
    /* reject overlong forms, surrogates and out-of-range code points */
    if ((len == 2 && cp < 0x80) || (len == 3 && cp < 0x800) ||
        (len == 4 && cp < 0x10000) || cp > 0x10FFFF ||
        (cp >= 0xD800 && cp <= 0xDFFF)) {
        return 0;
    }
    return len;
}

bool pc_buf_json_string_n(pc_buf_t *b, const char *s, size_t n)
{
    const unsigned char *p = (const unsigned char *)s;
    size_t i = 0;
    if (!pc_buf_append_n(b, "\"", 1)) return false;
    while (i < n) {
        unsigned char c = p[i];
        size_t run = i;
        /* copy the longest run of bytes needing no escape in one go */
        while (run < n && p[run] >= 0x20 && p[run] < 0x80 &&
               p[run] != '"' && p[run] != '\\') {
            ++run;
        }
        if (run > i) {
            pc_buf_append_n(b, p + i, run - i);
            i = run;
            continue;
        }
        if (c == '"') { pc_buf_append_n(b, "\\\"", 2); ++i; }
        else if (c == '\\') { pc_buf_append_n(b, "\\\\", 2); ++i; }
        else if (c == '\n') { pc_buf_append_n(b, "\\n", 2); ++i; }
        else if (c == '\r') { pc_buf_append_n(b, "\\r", 2); ++i; }
        else if (c == '\t') { pc_buf_append_n(b, "\\t", 2); ++i; }
        else if (c == '\b') { pc_buf_append_n(b, "\\b", 2); ++i; }
        else if (c == '\f') { pc_buf_append_n(b, "\\f", 2); ++i; }
        else if (c < 0x20) { pc_buf_appendf(b, "\\u%04x", c); ++i; }
        else {
            size_t len = utf8_seq(p + i, n - i);
            if (len == 0) {
                /* U+FFFD as raw UTF-8 (not "�"), so re-serialising the
                 * parsed output is a fixed point */
                pc_buf_append_n(b, "\xEF\xBF\xBD", 3);
                ++i;
            } else {
                pc_buf_append_n(b, p + i, len);
                i += len;
            }
        }
    }
    return pc_buf_append_n(b, "\"", 1);
}

bool pc_buf_json_string(pc_buf_t *b, const char *s)
{
    return pc_buf_json_string_n(b, s ? s : "", s ? strlen(s) : 0);
}

bool pc_buf_json_value(pc_buf_t *b, const json_value *v)
{
    size_t i;
    if (!v) return pc_buf_append(b, "null");
    switch (v->type) {
    case JSON_NULL:
        return pc_buf_append(b, "null");
    case JSON_BOOL:
        return pc_buf_append(b, v->boolean ? "true" : "false");
    case JSON_NUMBER:
        if (!isfinite(v->number)) return pc_buf_append(b, "null");
        if (v->is_integer && fabs(v->number) < 9007199254740992.0) {
            return pc_buf_appendf(b, "%.0f", v->number);
        }
        return pc_buf_appendf(b, "%.17g", v->number);
    case JSON_STRING:
        return pc_buf_json_string(b, v->string);
    case JSON_ARRAY:
        pc_buf_append(b, "[");
        for (i = 0; i < v->count; ++i) {
            if (i) pc_buf_append(b, ",");
            pc_buf_json_value(b, v->items[i]);
        }
        return pc_buf_append(b, "]");
    case JSON_OBJECT:
        pc_buf_append(b, "{");
        for (i = 0; i < v->count; ++i) {
            if (i) pc_buf_append(b, ",");
            pc_buf_json_string(b, v->keys[i]);
            pc_buf_append(b, ":");
            pc_buf_json_value(b, v->values[i]);
        }
        return pc_buf_append(b, "}");
    }
    return pc_buf_append(b, "null");
}

static const char B64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

bool pc_buf_base64(pc_buf_t *b, const void *data, size_t n)
{
    const unsigned char *p = (const unsigned char *)data;
    size_t i;
    char q[4];
    if (n > (SIZE_MAX / 4) * 3 - 3) { b->failed = true; return false; }
    if (!reserve(b, ((n + 2) / 3) * 4)) return false;
    for (i = 0; i + 3 <= n; i += 3) {
        uint32_t v = ((uint32_t)p[i] << 16) | ((uint32_t)p[i + 1] << 8) | p[i + 2];
        q[0] = B64[(v >> 18) & 63]; q[1] = B64[(v >> 12) & 63];
        q[2] = B64[(v >> 6) & 63];  q[3] = B64[v & 63];
        pc_buf_append_n(b, q, 4);
    }
    if (n - i == 1) {
        uint32_t v = (uint32_t)p[i] << 16;
        q[0] = B64[(v >> 18) & 63]; q[1] = B64[(v >> 12) & 63];
        q[2] = '='; q[3] = '=';
        pc_buf_append_n(b, q, 4);
    } else if (n - i == 2) {
        uint32_t v = ((uint32_t)p[i] << 16) | ((uint32_t)p[i + 1] << 8);
        q[0] = B64[(v >> 18) & 63]; q[1] = B64[(v >> 12) & 63];
        q[2] = B64[(v >> 6) & 63];  q[3] = '=';
        pc_buf_append_n(b, q, 4);
    }
    return !b->failed;
}

static int b64val(unsigned char c)
{
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

int pc_base64_decode(const char *in, size_t in_len, unsigned char **out,
                     size_t *out_len)
{
    size_t i, o = 0, groups;
    unsigned char *buf;
    *out = NULL;
    *out_len = 0;
    if (in_len % 4 != 0) return -1;
    groups = in_len / 4;
    buf = malloc(groups * 3 + 1);
    if (!buf) return -2;
    for (i = 0; i < groups; ++i) {
        const unsigned char *g = (const unsigned char *)in + i * 4;
        int a = b64val(g[0]), bb = b64val(g[1]), c, d;
        bool last = (i + 1 == groups);
        if (a < 0 || bb < 0) { free(buf); return -1; }
        if (last && g[2] == '=' && g[3] == '=') {
            if (bb & 0x0F) { free(buf); return -1; }      /* non-canonical */
            buf[o++] = (unsigned char)((a << 2) | (bb >> 4));
            break;
        }
        c = b64val(g[2]);
        if (c < 0) { free(buf); return -1; }
        if (last && g[3] == '=') {
            if (c & 0x03) { free(buf); return -1; }
            buf[o++] = (unsigned char)((a << 2) | (bb >> 4));
            buf[o++] = (unsigned char)(((bb & 0x0F) << 4) | (c >> 2));
            break;
        }
        d = b64val(g[3]);
        if (d < 0) { free(buf); return -1; }
        buf[o++] = (unsigned char)((a << 2) | (bb >> 4));
        buf[o++] = (unsigned char)(((bb & 0x0F) << 4) | (c >> 2));
        buf[o++] = (unsigned char)(((c & 0x03) << 6) | d);
    }
    buf[o] = '\0';
    *out = buf;
    *out_len = o;
    return 0;
}

size_t pc_copy_out(char *dst, size_t cap, const char *src)
{
    size_t n = src ? strlen(src) : 0;
    if (dst && cap) {
        size_t c = n < cap - 1 ? n : cap - 1;
        if (c) memcpy(dst, src, c);
        dst[c] = '\0';
    }
    return n;
}
