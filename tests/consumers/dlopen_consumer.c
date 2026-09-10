/*
 * Consumer #3 - explicit dynamic loading.
 * Uses no PolyCall headers at all: resolves polycall_get_version() at run
 * time via LoadLibrary/GetProcAddress (Windows) or dlopen/dlsym (POSIX).
 * The shared-library path is argv[1].
 */
#include <stdio.h>

typedef const char *(*version_fn)(void);

#if defined(_WIN32)
#include <windows.h>
int main(int argc, char **argv)
{
    HMODULE h;
    version_fn f;
    const char *v;
    if (argc < 2) { fprintf(stderr, "usage: %s <dll>\n", argv[0]); return 2; }
    h = LoadLibraryA(argv[1]);
    if (!h) { fprintf(stderr, "LoadLibrary(%s) failed: %lu\n", argv[1],
                      (unsigned long)GetLastError()); return 3; }
    f = (version_fn)(void *)GetProcAddress(h, "polycall_get_version");
    if (!f) { fprintf(stderr, "GetProcAddress(polycall_get_version) failed\n");
              FreeLibrary(h); return 4; }
    v = f();
    if (!v || !*v) { fprintf(stderr, "empty version\n"); FreeLibrary(h); return 5; }
    printf("%s\n", v);
    FreeLibrary(h);
    return 0;
}
#else
#include <dlfcn.h>
int main(int argc, char **argv)
{
    void *h;
    version_fn f;
    const char *v;
    if (argc < 2) { fprintf(stderr, "usage: %s <so>\n", argv[0]); return 2; }
    h = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!h) { fprintf(stderr, "dlopen(%s) failed: %s\n", argv[1], dlerror()); return 3; }
    *(void **)(&f) = dlsym(h, "polycall_get_version");
    if (!f) { fprintf(stderr, "dlsym(polycall_get_version) failed: %s\n", dlerror());
              dlclose(h); return 4; }
    v = f();
    if (!v || !*v) { fprintf(stderr, "empty version\n"); dlclose(h); return 5; }
    printf("%s\n", v);
    dlclose(h);
    return 0;
}
#endif
