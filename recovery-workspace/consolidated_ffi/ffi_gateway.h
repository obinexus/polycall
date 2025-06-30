/*
 * Sinphasé-Compliant FFI Gateway Header
 * Minimal interface, maximum 5 dependencies
 */

#ifndef FFI_GATEWAY_H
#define FFI_GATEWAY_H

// Minimal interface - single responsibility
int ffi_gateway_init(void);
int ffi_gateway_call(const char* language, const char* function, 
                     void* args, void* result);
void ffi_gateway_cleanup(void);

// Bridge creation functions (external)
struct bridge_s* c_bridge_create(void);
struct bridge_s* python_bridge_create(void);
struct bridge_s* js_bridge_create(void);
struct bridge_s* jvm_bridge_create(void);

#endif /* FFI_GATEWAY_H */
