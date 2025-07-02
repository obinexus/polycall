/*
 * security.h
 * Edge module security features
 */

#ifndef POLYCALL_EDGE_SECURITY_H
#define POLYCALL_EDGE_SECURITY_H

#include <stdbool.h>

typedef struct edge_security_context edge_security_context_t;

edge_security_context_t* edge_security_init(void);
void edge_security_cleanup(edge_security_context_t* ctx);
bool edge_security_validate(edge_security_context_t* ctx, const void* data, size_t len);

#endif /* POLYCALL_EDGE_SECURITY_H */
