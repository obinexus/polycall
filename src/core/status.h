#ifndef POLYCALL_INTERNAL_STATUS_H
#define POLYCALL_INTERNAL_STATUS_H

/*
 * Thread-local failure detail behind polycall_last_error(). Every public
 * binding-ABI entry point calls pc_err_clear() first and pc_err_set() on
 * failure, so the detail always describes the calling thread's own most
 * recent call. Internal; not installed.
 */

#include <stddef.h>

#define PC_ERR_MAX 512

void pc_err_clear(void);
/* sets the detail and returns `status` unchanged (for `return pc_err(...)`) */
int  pc_err(int status, const char *fmt, ...)
#if defined(__GNUC__)
    __attribute__((format(printf, 2, 3)))
#endif
    ;
const char *pc_err_text(void);

#endif /* POLYCALL_INTERNAL_STATUS_H */
