# syntax=docker/dockerfile:1.7

ARG POLYCALL_VERSION=1.0.1

FROM gcc:12-bookworm AS builder

WORKDIR /src

COPY Makefile Polycallfile ./
COPY include ./include
COPY src ./src

# `static` only builds libpolycall.a; the CLI needs the `cli` target too.
# Outputs land under BUILD_DIR (default `build/`), not bare bin/ and lib/.
RUN make clean && \
    make static cli && \
    strip build/bin/polycall

FROM debian:bookworm-slim AS runtime

ARG POLYCALL_VERSION

# The CLI is a dynamically linked glibc executable (ldd: libc.so.6 + the
# dynamic linker only -- no libpthread/libdl as separate objects on a modern
# glibc). `FROM scratch` cannot satisfy that; bookworm-slim matches the
# builder's glibc ABI and stays minimal (no packages need installing).
LABEL org.opencontainers.image.title="LibPolyCall" \
      org.opencontainers.image.description="Minimal LibPolyCall v1 runtime image" \
      org.opencontainers.image.version="${POLYCALL_VERSION}" \
      org.opencontainers.image.vendor="OBINexus" \
      org.opencontainers.image.licenses="MIT"

WORKDIR /app

COPY --from=builder --chmod=0555 /src/build/bin/polycall /usr/local/bin/polycall
COPY --from=builder --chmod=0444 /src/build/lib/libpolycall.a /usr/local/lib/libpolycall.a
COPY --from=builder /src/include /usr/local/include/libpolycall
COPY --from=builder --chmod=0444 /src/Polycallfile /etc/polycall/default.conf

USER 65532:65532

HEALTHCHECK --interval=30s --timeout=5s --start-period=5s --retries=3 \
    CMD ["/usr/local/bin/polycall", "doctor"]

ENTRYPOINT ["/usr/local/bin/polycall"]
CMD ["--help"]
