#include "status.h"

#include "polycall.h"
#include "pc_buf.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#if defined(_MSC_VER)
#  define PC_TLS __declspec(thread)
#else
#  define PC_TLS _Thread_local
#endif

static PC_TLS char g_err[PC_ERR_MAX];

void pc_err_clear(void)
{
    g_err[0] = '\0';
}

int pc_err(int status, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(g_err, sizeof g_err, fmt, ap);
    va_end(ap);
    return status;
}

const char *pc_err_text(void)
{
    return g_err;
}

int polycall_ffi_abi_version(void)
{
    return POLYCALL_FFI_ABI_VERSION;
}

int polycall_ffi_version(char *buf, int len)
{
    if (len < 0 || (len > 0 && !buf)) {
        return POLYCALL_E_INVALID_ARGUMENT;
    }
    return (int)pc_copy_out(buf, (size_t)len, POLYCALL_ABI_VERSION_STRING);
}

const char *polycall_strerror(int status)
{
    switch (status) {
    case POLYCALL_OK:                  return "POLYCALL_OK: success";
    case POLYCALL_E_INVALID_ARGUMENT:  return "POLYCALL_E_INVALID_ARGUMENT: invalid argument";
    case POLYCALL_E_NO_MEMORY:         return "POLYCALL_E_NO_MEMORY: out of memory";
    case POLYCALL_E_INVALID_HANDLE:    return "POLYCALL_E_INVALID_HANDLE: unknown, closed or stale handle";
    case POLYCALL_E_TIMEOUT:           return "POLYCALL_E_TIMEOUT: deadline exceeded";
    case POLYCALL_E_TRANSPORT:         return "POLYCALL_E_TRANSPORT: peer unreachable, refused or reset";
    case POLYCALL_E_PROTOCOL:          return "POLYCALL_E_PROTOCOL: malformed or unexpected reply";
    case POLYCALL_E_NOT_FOUND:         return "POLYCALL_E_NOT_FOUND: not found";
    case POLYCALL_E_AUTH:              return "POLYCALL_E_AUTH: authentication failed";
    case POLYCALL_E_REMOTE:            return "POLYCALL_E_REMOTE: remote side reported an error";
    case POLYCALL_E_TOO_LARGE:         return "POLYCALL_E_TOO_LARGE: exceeds a limit or the buffer";
    case POLYCALL_E_BUSY:              return "POLYCALL_E_BUSY: queue or connection limit reached";
    case POLYCALL_E_CANCELLED:         return "POLYCALL_E_CANCELLED: cancelled";
    case POLYCALL_E_CONFIG:            return "POLYCALL_E_CONFIG: invalid configuration";
    case POLYCALL_E_ADDRESS_IN_USE:    return "POLYCALL_E_ADDRESS_IN_USE: address already in use";
    case POLYCALL_E_UNSUPPORTED:       return "POLYCALL_E_UNSUPPORTED: not supported by this build";
    case POLYCALL_E_PERMISSION:        return "POLYCALL_E_PERMISSION: permission denied";
    case POLYCALL_E_CLOSED:            return "POLYCALL_E_CLOSED: handle closed";
    case POLYCALL_E_INTERNAL:          return "POLYCALL_E_INTERNAL: internal error";
    default:                           return "POLYCALL_E_UNKNOWN: unknown status code";
    }
}

int polycall_last_error(char *buf, size_t cap)
{
    return (int)pc_copy_out(buf, cap, g_err);
}
