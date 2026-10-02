/* Installed-library consumer: public header only, real calls, two peer
 * nodes exchanging a payload over loopback. Prints the version on success. */
#include <polycall.h>

#include <stdio.h>
#include <string.h>

int main(void)
{
    polycall_peer_t a = 0, b = 0;
    char ep[POLYCALL_ENDPOINT_MAX], from[POLYCALL_PEER_ID_MAX], mid[POLYCALL_MESSAGE_ID_MAX];
    char buf[64];
    size_t len = 0;
    int rc;

    if (polycall_ffi_abi_version() != POLYCALL_FFI_ABI_VERSION) {
        fprintf(stderr, "ABI mismatch: header %d library %d\n",
                POLYCALL_FFI_ABI_VERSION, polycall_ffi_abi_version());
        return 1;
    }
    if (strcmp(polycall_get_version(), POLYCALL_ABI_VERSION_STRING) != 0) {
        fprintf(stderr, "header %s vs library %s\n", POLYCALL_ABI_VERSION_STRING,
                polycall_get_version());
        return 1;
    }
    rc = polycall_peer_open("installed-a", "127.0.0.1:0", "t", &a);
    if (rc == POLYCALL_OK) rc = polycall_peer_open("installed-b", "127.0.0.1:0", "t", &b);
    if (rc == POLYCALL_OK) rc = polycall_peer_endpoint(b, ep, sizeof ep);
    if (rc == POLYCALL_OK) rc = polycall_peer_send(a, ep, "installed", 9, "m1", 3000);
    if (rc == POLYCALL_OK) {
        rc = polycall_peer_recv(b, 3000, from, sizeof from, mid, sizeof mid,
                                buf, sizeof buf, &len);
    }
    if (rc != POLYCALL_OK || len != 9 || memcmp(buf, "installed", 9) != 0 ||
        strcmp(from, "installed-a") != 0 || strcmp(mid, "m1") != 0) {
        char e[256];
        polycall_last_error(e, sizeof e);
        fprintf(stderr, "exchange failed: %s (%s)\n", polycall_strerror(rc), e);
        return 1;
    }
    polycall_peer_close(a);
    polycall_peer_close(b);
    printf("%s\n", polycall_get_version());
    return 0;
}
