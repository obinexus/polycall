#ifndef POLYCALL_INTERNAL_JSON_H
#define POLYCALL_INTERNAL_JSON_H

/*
 * Tiny read-only JSON DOM. Internal to the config layer; not installed.
 * Enough for the provider envelope: objects, arrays, strings, numbers
 * (double + exact-integer flag), booleans, null. No comments, no trailing
 * commas, UTF-8 passed through, \uXXXX (incl. surrogate pairs) decoded.
 */

#include <stddef.h>
#include <stdbool.h>

typedef enum {
    JSON_NULL, JSON_BOOL, JSON_NUMBER, JSON_STRING, JSON_ARRAY, JSON_OBJECT
} json_type_t;

typedef struct json_value json_value;

struct json_value {
    json_type_t type;
    /* JSON_BOOL   */ bool boolean;
    /* JSON_NUMBER */ double number; bool is_integer;
    /* JSON_STRING */ char *string;          /* NUL-terminated, decoded     */
    /* JSON_ARRAY  */ json_value **items; size_t count;
    /* JSON_OBJECT */ char **keys; json_value **values; /* count reused     */
};

/* Parse NUL-terminated / length-bounded text. Returns NULL on error and,
 * when errbuf != NULL, writes a one-line reason. Free with json_free(). */
json_value *json_parse(const char *text, size_t len, char *errbuf, size_t errcap);
void        json_free(json_value *v);

/* object lookup (returns NULL if absent or v is not an object) */
const json_value *json_get(const json_value *v, const char *key);

/* typed accessors with defaults; *ok (optional) says whether the type matched */
const char *json_str(const json_value *v, const char *dflt, bool *ok);
double      json_num(const json_value *v, double dflt, bool *ok);
bool        json_bool(const json_value *v, bool dflt, bool *ok);

#endif /* POLYCALL_INTERNAL_JSON_H */
