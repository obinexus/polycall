#include "json.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *p;
    const char *end;
    char *err;
    size_t errcap;
    int depth;
} parser;

#define JSON_MAX_DEPTH 64

static void fail(parser *ps, const char *msg)
{
    if (ps->err && ps->errcap && ps->err[0] == '\0') {
        snprintf(ps->err, ps->errcap, "json: %s", msg);
    }
}

static json_value *pv_new(json_type_t t)
{
    json_value *v = calloc(1, sizeof *v);
    if (v) {
        v->type = t;
    }
    return v;
}

void json_free(json_value *v)
{
    size_t i;
    if (!v) {
        return;
    }
    switch (v->type) {
    case JSON_STRING:
        free(v->string);
        break;
    case JSON_ARRAY:
        for (i = 0; i < v->count; ++i) {
            json_free(v->items[i]);
        }
        free(v->items);
        break;
    case JSON_OBJECT:
        for (i = 0; i < v->count; ++i) {
            free(v->keys[i]);
            json_free(v->values[i]);
        }
        free(v->keys);
        free(v->values);
        break;
    default:
        break;
    }
    free(v);
}

static void skip_ws(parser *ps)
{
    while (ps->p < ps->end) {
        char c = *ps->p;
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            ps->p++;
        } else {
            break;
        }
    }
}

static json_value *parse_value(parser *ps);

static void utf8_emit(char **w, unsigned long cp)
{
    if (cp < 0x80) {
        *(*w)++ = (char)cp;
    } else if (cp < 0x800) {
        *(*w)++ = (char)(0xC0 | (cp >> 6));
        *(*w)++ = (char)(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        *(*w)++ = (char)(0xE0 | (cp >> 12));
        *(*w)++ = (char)(0x80 | ((cp >> 6) & 0x3F));
        *(*w)++ = (char)(0x80 | (cp & 0x3F));
    } else {
        *(*w)++ = (char)(0xF0 | (cp >> 18));
        *(*w)++ = (char)(0x80 | ((cp >> 12) & 0x3F));
        *(*w)++ = (char)(0x80 | ((cp >> 6) & 0x3F));
        *(*w)++ = (char)(0x80 | (cp & 0x3F));
    }
}

static int hex4(const char *s, unsigned *out)
{
    unsigned v = 0;
    int i;
    for (i = 0; i < 4; ++i) {
        char c = s[i];
        v <<= 4;
        if (c >= '0' && c <= '9') v |= (unsigned)(c - '0');
        else if (c >= 'a' && c <= 'f') v |= (unsigned)(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') v |= (unsigned)(c - 'A' + 10);
        else return -1;
    }
    *out = v;
    return 0;
}

/* parses a JSON string starting at ps->p == '"' */
static char *parse_string_raw(parser *ps)
{
    const char *s;
    char *out, *w;
    size_t cap;

    ps->p++; /* opening quote */
    s = ps->p;
    cap = 16;
    out = malloc(cap);
    if (!out) {
        fail(ps, "oom");
        return NULL;
    }
    w = out;

    while (ps->p < ps->end) {
        unsigned char c = (unsigned char)*ps->p;
        /* grow: worst case one input byte -> up to 4 output bytes */
        if ((size_t)(w - out) + 8 > cap) {
            size_t used = (size_t)(w - out);
            char *bigger = realloc(out, cap *= 2);
            if (!bigger) { free(out); fail(ps, "oom"); return NULL; }
            out = bigger;
            w = out + used;
        }
        if (c == '"') {
            ps->p++;
            *w = '\0';
            return out;
        }
        if (c == '\\') {
            ps->p++;
            if (ps->p >= ps->end) break;
            switch (*ps->p) {
            case '"':  *w++ = '"';  ps->p++; break;
            case '\\': *w++ = '\\'; ps->p++; break;
            case '/':  *w++ = '/';  ps->p++; break;
            case 'b':  *w++ = '\b'; ps->p++; break;
            case 'f':  *w++ = '\f'; ps->p++; break;
            case 'n':  *w++ = '\n'; ps->p++; break;
            case 'r':  *w++ = '\r'; ps->p++; break;
            case 't':  *w++ = '\t'; ps->p++; break;
            case 'u': {
                unsigned cp;
                if (ps->p + 5 > ps->end || hex4(ps->p + 1, &cp) != 0) {
                    free(out); fail(ps, "bad \\u escape"); return NULL;
                }
                ps->p += 5;
                if (cp >= 0xD800 && cp <= 0xDBFF) { /* high surrogate */
                    unsigned lo;
                    if (ps->p + 6 > ps->end || ps->p[0] != '\\' || ps->p[1] != 'u'
                        || hex4(ps->p + 2, &lo) != 0 || lo < 0xDC00 || lo > 0xDFFF) {
                        free(out); fail(ps, "bad surrogate pair"); return NULL;
                    }
                    ps->p += 6;
                    utf8_emit(&w, 0x10000UL + (((unsigned long)cp - 0xD800) << 10)
                                            + ((unsigned long)lo - 0xDC00));
                } else {
                    utf8_emit(&w, cp);
                }
                break;
            }
            default:
                free(out); fail(ps, "bad escape"); return NULL;
            }
        } else if (c < 0x20) {
            free(out); fail(ps, "control char in string"); return NULL;
        } else {
            *w++ = (char)c;
            ps->p++;
        }
    }
    (void)s;
    free(out);
    fail(ps, "unterminated string");
    return NULL;
}

static json_value *parse_string(parser *ps)
{
    char *str = parse_string_raw(ps);
    json_value *v;
    if (!str) {
        return NULL;
    }
    v = pv_new(JSON_STRING);
    if (!v) { free(str); fail(ps, "oom"); return NULL; }
    v->string = str;
    return v;
}

static json_value *parse_number(parser *ps)
{
    const char *start = ps->p;
    char buf[64];
    size_t n;
    json_value *v;
    bool is_int = true;

    if (ps->p < ps->end && *ps->p == '-') ps->p++;
    while (ps->p < ps->end && isdigit((unsigned char)*ps->p)) ps->p++;
    if (ps->p < ps->end && *ps->p == '.') {
        is_int = false;
        ps->p++;
        while (ps->p < ps->end && isdigit((unsigned char)*ps->p)) ps->p++;
    }
    if (ps->p < ps->end && (*ps->p == 'e' || *ps->p == 'E')) {
        is_int = false;
        ps->p++;
        if (ps->p < ps->end && (*ps->p == '+' || *ps->p == '-')) ps->p++;
        while (ps->p < ps->end && isdigit((unsigned char)*ps->p)) ps->p++;
    }
    n = (size_t)(ps->p - start);
    if (n == 0 || n >= sizeof buf) {
        fail(ps, "bad number");
        return NULL;
    }
    memcpy(buf, start, n);
    buf[n] = '\0';

    v = pv_new(JSON_NUMBER);
    if (!v) { fail(ps, "oom"); return NULL; }
    v->number = strtod(buf, NULL);
    v->is_integer = is_int && (v->number == floor(v->number))
                    && fabs(v->number) < 9.007199254740992e15;
    return v;
}

static json_value *parse_array(parser *ps)
{
    json_value *v = pv_new(JSON_ARRAY);
    if (!v) { fail(ps, "oom"); return NULL; }
    ps->p++; /* [ */
    skip_ws(ps);
    if (ps->p < ps->end && *ps->p == ']') { ps->p++; return v; }
    for (;;) {
        json_value *item;
        json_value **grow;
        skip_ws(ps);
        item = parse_value(ps);
        if (!item) { json_free(v); return NULL; }
        grow = realloc(v->items, (v->count + 1) * sizeof *v->items);
        if (!grow) { json_free(item); json_free(v); fail(ps, "oom"); return NULL; }
        v->items = grow;
        v->items[v->count++] = item;
        skip_ws(ps);
        if (ps->p >= ps->end) { json_free(v); fail(ps, "unterminated array"); return NULL; }
        if (*ps->p == ',') { ps->p++; continue; }
        if (*ps->p == ']') { ps->p++; return v; }
        json_free(v);
        fail(ps, "expected ',' or ']'");
        return NULL;
    }
}

static json_value *parse_object(parser *ps)
{
    json_value *v = pv_new(JSON_OBJECT);
    if (!v) { fail(ps, "oom"); return NULL; }
    ps->p++; /* { */
    skip_ws(ps);
    if (ps->p < ps->end && *ps->p == '}') { ps->p++; return v; }
    for (;;) {
        char *key;
        json_value *val;
        char **gk;
        json_value **gv;
        skip_ws(ps);
        if (ps->p >= ps->end || *ps->p != '"') { json_free(v); fail(ps, "expected key string"); return NULL; }
        key = parse_string_raw(ps);
        if (!key) { json_free(v); return NULL; }
        skip_ws(ps);
        if (ps->p >= ps->end || *ps->p != ':') { free(key); json_free(v); fail(ps, "expected ':'"); return NULL; }
        ps->p++;
        skip_ws(ps);
        val = parse_value(ps);
        if (!val) { free(key); json_free(v); return NULL; }
        gk = realloc(v->keys, (v->count + 1) * sizeof *v->keys);
        if (!gk) { free(key); json_free(val); json_free(v); fail(ps, "oom"); return NULL; }
        v->keys = gk;
        gv = realloc(v->values, (v->count + 1) * sizeof *v->values);
        if (!gv) { free(key); json_free(val); json_free(v); fail(ps, "oom"); return NULL; }
        v->values = gv;
        v->keys[v->count] = key;
        v->values[v->count] = val;
        v->count++;
        skip_ws(ps);
        if (ps->p >= ps->end) { json_free(v); fail(ps, "unterminated object"); return NULL; }
        if (*ps->p == ',') { ps->p++; continue; }
        if (*ps->p == '}') { ps->p++; return v; }
        json_free(v);
        fail(ps, "expected ',' or '}'");
        return NULL;
    }
}

static json_value *parse_value(parser *ps)
{
    skip_ws(ps);
    if (ps->p >= ps->end) { fail(ps, "unexpected end"); return NULL; }
    if (++ps->depth > JSON_MAX_DEPTH) { fail(ps, "too deep"); return NULL; }

    json_value *r = NULL;
    switch (*ps->p) {
    case '{': r = parse_object(ps); break;
    case '[': r = parse_array(ps); break;
    case '"': r = parse_string(ps); break;
    case 't':
        if (ps->end - ps->p >= 4 && memcmp(ps->p, "true", 4) == 0) {
            ps->p += 4; r = pv_new(JSON_BOOL); if (r) r->boolean = true;
        } else fail(ps, "bad literal");
        break;
    case 'f':
        if (ps->end - ps->p >= 5 && memcmp(ps->p, "false", 5) == 0) {
            ps->p += 5; r = pv_new(JSON_BOOL); if (r) r->boolean = false;
        } else fail(ps, "bad literal");
        break;
    case 'n':
        if (ps->end - ps->p >= 4 && memcmp(ps->p, "null", 4) == 0) {
            ps->p += 4; r = pv_new(JSON_NULL);
        } else fail(ps, "bad literal");
        break;
    default:
        if (*ps->p == '-' || isdigit((unsigned char)*ps->p)) {
            r = parse_number(ps);
        } else {
            fail(ps, "unexpected character");
        }
    }
    ps->depth--;
    return r;
}

json_value *json_parse(const char *text, size_t len, char *errbuf, size_t errcap)
{
    parser ps;
    json_value *v;

    if (errbuf && errcap) {
        errbuf[0] = '\0';
    }
    if (!text) {
        if (errbuf && errcap) snprintf(errbuf, errcap, "json: NULL input");
        return NULL;
    }
    ps.p = text;
    ps.end = text + len;
    ps.err = errbuf;
    ps.errcap = errcap;
    ps.depth = 0;

    v = parse_value(&ps);
    if (!v) {
        return NULL;
    }
    skip_ws(&ps);
    if (ps.p != ps.end) {
        json_free(v);
        if (errbuf && errcap && errbuf[0] == '\0') {
            snprintf(errbuf, errcap, "json: trailing data");
        }
        return NULL;
    }
    return v;
}

const json_value *json_get(const json_value *v, const char *key)
{
    size_t i;
    if (!v || v->type != JSON_OBJECT || !key) {
        return NULL;
    }
    for (i = 0; i < v->count; ++i) {
        if (strcmp(v->keys[i], key) == 0) {
            return v->values[i];
        }
    }
    return NULL;
}

const char *json_str(const json_value *v, const char *dflt, bool *ok)
{
    if (v && v->type == JSON_STRING) { if (ok) *ok = true; return v->string; }
    if (ok) *ok = false;
    return dflt;
}

double json_num(const json_value *v, double dflt, bool *ok)
{
    if (v && v->type == JSON_NUMBER) { if (ok) *ok = true; return v->number; }
    if (ok) *ok = false;
    return dflt;
}

bool json_bool(const json_value *v, bool dflt, bool *ok)
{
    if (v && v->type == JSON_BOOL) { if (ok) *ok = true; return v->boolean; }
    if (ok) *ok = false;
    return dflt;
}
