#ifndef POLYCALL_EXPORT_H
#define POLYCALL_EXPORT_H

/*
 * Public linkage / visibility control for the PolyCall C ABI.
 *
 * One header, usable by both the GNU Make build and the CMake build, and by
 * external consumers:
 *
 *   - Building the shared library ...... define POLYCALL_BUILD_SHARED
 *   - Consuming the shared library ..... define POLYCALL_USE_SHARED
 *   - Static linkage (default) ......... define nothing (or POLYCALL_STATIC)
 *
 * POLYCALL_API   - decorates every public symbol.
 * POLYCALL_LOCAL - hides a symbol from the shared-object export table (ELF).
 * POLYCALL_CALL  - the C calling convention every public function uses.
 *
 * The macros never change a struct layout and never pull in a header, so a
 * translation unit that does not care about DLL linkage can ignore them.
 */

#if defined(POLYCALL_BUILD_SHARED) && defined(POLYCALL_USE_SHARED)
#error "Define at most one of POLYCALL_BUILD_SHARED / POLYCALL_USE_SHARED"
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
#  define POLYCALL_IMPORT __declspec(dllimport)
#  define POLYCALL_EXPORT __declspec(dllexport)
#  define POLYCALL_HIDDEN
#else
#  if defined(__GNUC__) && (__GNUC__ >= 4)
#    define POLYCALL_IMPORT __attribute__((visibility("default")))
#    define POLYCALL_EXPORT __attribute__((visibility("default")))
#    define POLYCALL_HIDDEN __attribute__((visibility("hidden")))
#  else
#    define POLYCALL_IMPORT
#    define POLYCALL_EXPORT
#    define POLYCALL_HIDDEN
#  endif
#endif

#if defined(POLYCALL_BUILD_SHARED)
#  define POLYCALL_API POLYCALL_EXPORT
#  define POLYCALL_LOCAL POLYCALL_HIDDEN
#elif defined(POLYCALL_USE_SHARED)
#  define POLYCALL_API POLYCALL_IMPORT
#  define POLYCALL_LOCAL
#else
/* static linkage */
#  define POLYCALL_API
#  define POLYCALL_LOCAL
#endif

/* Public functions use the platform default C calling convention explicitly so
 * a consumer built with a different default (e.g. /Gz on MSVC) still links. */
#if defined(_WIN32) && !defined(_WIN64)
#  define POLYCALL_CALL __cdecl
#else
#  define POLYCALL_CALL
#endif

#ifdef __cplusplus
#  define POLYCALL_BEGIN_DECLS extern "C" {
#  define POLYCALL_END_DECLS }
#else
#  define POLYCALL_BEGIN_DECLS
#  define POLYCALL_END_DECLS
#endif

/*
 * Version triplet. These describe *three independent* contracts; do not
 * collapse them:
 *   - POLYCALL_ABI_VERSION_*    the compiled library's C ABI
 *   - configuration schema version   -> see polycall_config2.h (Stage 2)
 *   - wire protocol version          -> see polycall_protocol.h  (Stage 3)
 */
#define POLYCALL_ABI_VERSION_MAJOR 1
#define POLYCALL_ABI_VERSION_MINOR 0
#define POLYCALL_ABI_VERSION_PATCH 1
#define POLYCALL_ABI_VERSION_STRING "1.0.1"

#endif /* POLYCALL_EXPORT_H */
