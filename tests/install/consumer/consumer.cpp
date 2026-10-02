// Installed-library consumer in C++: the C header is usable from C++ as-is.
#include <polycall.h>

#include <cstdio>
#include <string>

int main()
{
    char v[32];
    if (polycall_ffi_version(v, sizeof v) <= 0) return 1;
    std::string s(v);
    if (s != POLYCALL_ABI_VERSION_STRING) {
        std::fprintf(stderr, "version mismatch %s\n", v);
        return 1;
    }
    std::printf("%s\n", s.c_str());
    return 0;
}
