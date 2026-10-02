#ifndef POLYCALL_INTERNAL_PC_BUF_H
#define POLYCALL_INTERNAL_PC_BUF_H

/*
 * Growable byte buffer + the encoders every wire path needs: JSON string
 * escaping, base64, and an exact serialiser for the internal JSON DOM.
 * Internal; not installed.
 *
 * Failure is sticky: once an append fails (OOM or the configured limit),
 * every later append is a no-op and pc_buf_failed() reports it, so callers
 * check once at the end instead of after every append -- and a truncated
 * buffer can never be mistaken for a complete one.
 */

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

struct json_value;

typedef struct {
    char  *data;     /* always NUL-terminated when len > 0 or after init */
    size_t len;
    size_t cap;
    size_t limit;    /* 0 = unlimited; otherwise max len (excluding NUL) */
    bool   failed;
} pc_buf_t;

void   pc_buf_init(pc_buf_t *b, size_t limit);
void   pc_buf_free(pc_buf_t *b);
void   pc_buf_reset(pc_buf_t *b);           /* len = 0, keeps capacity, clears failure */
bool   pc_buf_failed(const pc_buf_t *b);
/* Detach the data (caller frees with free()); buffer is re-initialised. */
char  *pc_buf_take(pc_buf_t *b, size_t *len);

bool pc_buf_append_n(pc_buf_t *b, const void *data, size_t n);
bool pc_buf_append(pc_buf_t *b, const char *s);
bool pc_buf_appendf(pc_buf_t *b, const char *fmt, ...)
#if defined(__GNUC__)
    __attribute__((format(printf, 2, 3)))
#endif
    ;
bool pc_buf_vappendf(pc_buf_t *b, const char *fmt, va_list ap);

/* Append `s` as a quoted JSON string literal (control characters, quotes,
 * backslashes escaped; UTF-8 passed through; invalid UTF-8 bytes replaced
 * with U+FFFD so the output is always valid JSON). */
bool pc_buf_json_string(pc_buf_t *b, const char *s);
bool pc_buf_json_string_n(pc_buf_t *b, const char *s, size_t n);

/* Exact compact serialisation of a parsed DOM value (objects keep their
 * member order; integers that were exact stay integers). */
bool pc_buf_json_value(pc_buf_t *b, const struct json_value *v);

/* base64 (RFC 4648, standard alphabet, padded). */
bool pc_buf_base64(pc_buf_t *b, const void *data, size_t n);
/* Strict decode: rejects bad characters, bad padding, and non-canonical
 * trailing bits. Returns 0 on success, -1 on malformed input, -2 on OOM.
 * *out is malloc'd (caller frees; also when *out_len == 0). */
int  pc_base64_decode(const char *in, size_t in_len, unsigned char **out,
                      size_t *out_len);

/* Copy `src` into a caller buffer with snprintf semantics. Returns the
 * full length of src (excluding NUL). */
size_t pc_copy_out(char *dst, size_t cap, const char *src);

#endif /* POLYCALL_INTERNAL_PC_BUF_H */
