// C++ consumer of the C ABI: the public header must compile as C++ (extern
// "C" guards, no C-only constructs) and link against the shared library.
// Prints exactly the library version on success, like the C consumers.

#include <polycall.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

int main()
{
    if (polycall_ffi_abi_version() != POLYCALL_FFI_ABI_VERSION) {
        std::fprintf(stderr, "ABI mismatch: header %d, library %d\n",
                     POLYCALL_FFI_ABI_VERSION, polycall_ffi_abi_version());
        return 1;
    }
    std::vector<char> buf(static_cast<size_t>(polycall_ffi_version(nullptr, 0)) + 1);
    polycall_ffi_version(buf.data(), static_cast<int>(buf.size()));
    std::string v(buf.data());
    if (v != polycall_get_version()) {
        std::fprintf(stderr, "version functions disagree\n");
        return 1;
    }
    // RAII-style use of a peer handle through the C ABI
    polycall_peer_t h = 0;
    int rc = polycall_peer_open("cxx-consumer", nullptr, nullptr, &h);
    if (rc != POLYCALL_OK) {
        std::fprintf(stderr, "peer_open: %s\n", polycall_strerror(rc));
        return 1;
    }
    char list[64];
    size_t n = 0;
    rc = polycall_peer_list(h, list, sizeof list, &n);
    polycall_peer_close(h);
    if (rc != POLYCALL_OK || std::strcmp(list, "{}") != 0) {
        std::fprintf(stderr, "peer_list: %s\n", polycall_strerror(rc));
        return 1;
    }
    std::printf("%s\n", v.c_str());
    return 0;
}
