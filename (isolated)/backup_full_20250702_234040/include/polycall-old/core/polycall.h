#ifndef LIBPOLYCALL_H
#define LIBPOLYCALL_H

#ifdef __cplusplus
extern "C" {
#endif

/* LibPolyCall Core API */
typedef struct polycall_runtime polycall_runtime_t;

polycall_runtime_t* polycall_runtime_create(const char* config_path);
void polycall_runtime_destroy(polycall_runtime_t* runtime);

#ifdef __cplusplus
}
#endif

#endif /* LIBPOLYCALL_H */
