#ifndef POLYCALL_INTERNAL_PC_FILE_H
#define POLYCALL_INTERNAL_PC_FILE_H

/*
 * fopen() for a UTF-8 path. Internal; not installed.
 *
 * On Windows the C runtime's narrow fopen() interprets the path in the
 * process's ANSI code page. polycall.exe sets that to UTF-8 through its
 * manifest (src/polycall.manifest), but a process that loads polycall.dll
 * (a JVM, .NET, Node, Python ...) does not, so a UTF-8 path with non-ASCII
 * bytes would not resolve. pc_fopen converts the path and mode to UTF-16
 * and calls _wfopen. Everywhere else it is fopen().
 */

#include <stdio.h>

FILE *pc_fopen(const char *path, const char *mode);

#endif /* POLYCALL_INTERNAL_PC_FILE_H */
