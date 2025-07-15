#ifndef POLYCALL_POLYCALL_POLYCALL_PERMISSION_H_H
#include <stdint.h>

#define POLYCALL_POLYCALL_POLYCALL_PERMISSION_H_H


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Permission set type for controlling access rights
 */
typedef uint32_t polycall_permission_set_t;


/**
 * @brief Permission flags
 */
enum {
    POLYCALL_PERM_NONE      = 0x00000000,  /**< No permissions */
    POLYCALL_PERM_READ      = 0x00000001,  /**< Read permission */
    POLYCALL_PERM_WRITE     = 0x00000002,  /**< Write permission */
    POLYCALL_PERM_EXECUTE   = 0x00000004,  /**< Execute permission */
    POLYCALL_PERM_ALL       = 0x00000007   /**< All permissions */
};

/**
 * @brief Check if a permission set has a specific permission
 * @param set The permission set to check
 * @param perm The permission to check for
 * @return 1 if permission exists, 0 otherwise
 */
static inline int polycall_permission_has(polycall_permission_set_t set, polycall_permission_set_t perm) {
    return (set & perm) == perm;
}

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_POLYCALL_POLYCALL_PERMISSION_H_H */