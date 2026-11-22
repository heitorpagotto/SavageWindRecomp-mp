#include "random.h"

#include <stdint.h>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <bcrypt.h>
#else
#include <errno.h>
#include <sys/random.h>
#endif

bool swmp_random_bytes(void *out, size_t size) {
#ifdef _WIN32
    return size <= UINT32_MAX && BCryptGenRandom(NULL, out, (ULONG)size,
                                                 BCRYPT_USE_SYSTEM_PREFERRED_RNG) == 0;
#else
    uint8_t *p = out;
    while (size) {
        ssize_t n = getrandom(p, size, 0);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return false;
        p += n;
        size -= (size_t)n;
    }
    return true;
#endif
}
