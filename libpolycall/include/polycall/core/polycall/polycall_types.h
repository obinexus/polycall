#ifndef POLYCALL_CORE_POLYCALL_TYPES_H
#define POLYCALL_CORE_POLYCALL_TYPES_H

#include <stddef.h>
#include <stdint.h>

/* Basic polycall types */
typedef struct polycall_context polycall_context_t;
typedef struct polycall_token polycall_token_t;
typedef struct polycall_identifier polycall_identifier_t;

/* Communication types */
typedef struct polycall_request polycall_request_t;
typedef struct polycall_response polycall_response_t;

/* Define maximum sizes */
#define POLYCALL_MAX_NAME_LENGTH 256
#define POLYCALL_MAX_PATH_LENGTH 1024

#endif /* POLYCALL_CORE_POLYCALL_TYPES_H */