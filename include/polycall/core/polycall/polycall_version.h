#ifndef POLYCALL_POLYCALL_POLYCALL_VERSION_H_H
#define POLYCALL_POLYCALL_POLYCALL_VERSION_H_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct polycall_version {
  int major;
  int minor;
  int patch;
  const char *string;
} polycall_version_t;

polycall_version_t polycall_get_version(void);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_POLYCALL_POLYCALL_VERSION_H_H */
