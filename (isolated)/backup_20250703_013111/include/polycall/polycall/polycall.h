/*
 * polycall.h
 * Main PolyCall runtime header
 */

#ifndef POLYCALL_POLYCALL_H
#define POLYCALL_POLYCALL_H

#include <stddef.h>

typedef struct polycall_context polycall_context_t;

polycall_context_t* polycall_init(void);
void polycall_cleanup(polycall_context_t* ctx);
int polycall_execute(polycall_context_t* ctx, const char* command, void* params);

#endif /* POLYCALL_POLYCALL_H */
