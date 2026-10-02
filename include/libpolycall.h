#ifndef LIBPOLYCALL_H
#define LIBPOLYCALL_H

/*
 * DEPRECATED compatibility header.
 *
 * Older consumers of the library (published as "libpolycall") included
 * <libpolycall.h>. The public consumer header is now <polycall.h>; this
 * file only forwards to it so those consumers keep compiling, and it will
 * be removed in a future major version. Replace
 *
 *     #include <libpolycall.h>      with      #include <polycall.h>
 *
 * Define POLYCALL_NO_DEPRECATION_WARNINGS to silence the notice.
 */

#if !defined(POLYCALL_NO_DEPRECATION_WARNINGS)
#  if defined(_MSC_VER)
#    pragma message("libpolycall.h is deprecated; include <polycall.h> instead")
#  elif defined(__GNUC__) || defined(__clang__)
#    warning "libpolycall.h is deprecated; include <polycall.h> instead"
#  endif
#endif

#include "polycall.h"

#endif /* LIBPOLYCALL_H */
