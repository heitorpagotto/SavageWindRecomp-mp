#include "package_marker.h"

#include "protocol.h"
#include "services/archive.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static const char marker_path[] = "System/SavageWind/Multiplayer/package.swmp";
static const char marker[] =
    "SWMP_PACKAGE_V1\n"
    "id = org.savagewind.multiplayer\n"
    "protocol = 1\n"
    "runtime = native\n";

_Static_assert(SWMP_PROTOCOL_VERSION == 1, "Update the marker and package builder for the new protocol");

bool swmp_package_marker_enabled(const char *overlay) {
    if (!overlay || !*overlay) return false;

    char path[1200];
    int length = snprintf(path, sizeof path, "%s/%s", overlay, marker_path);
    if (length < 0 || (size_t)length >= sizeof path) return false;

    // Leave room for Yaz0 literal overhead and alignment, but never allocate from a file's claimed size.
    uint8_t data[256];
    struct stat st;
    if (stat(path, &st) != 0 || !S_ISREG(st.st_mode) || st.st_size <= 0 ||
        (uint64_t)st.st_size >= sizeof data) return false;
    FILE *file = fopen(path, "rb");
    if (!file) return false;
    size_t size = fread(data, 1, sizeof data, file);
    bool complete = feof(file) && !ferror(file);
    fclose(file);
    if (!complete) return false;

    const size_t marker_size = sizeof marker - 1;
    if (size == marker_size && !memcmp(data, marker, marker_size)) return true;
    if (size < 16 || memcmp(data, "Yaz0", 4)) return false;
    uint32_t unpacked_size = (uint32_t)data[4] << 24 | (uint32_t)data[5] << 16 |
                             (uint32_t)data[6] << 8 | data[7];
    if (unpacked_size != marker_size) return false;

    size_t decoded_size;
    uint8_t *decoded = yaz0_unpack(data, size, &decoded_size);
    bool enabled = decoded && decoded_size == marker_size && !memcmp(decoded, marker, marker_size);
    free(decoded);
    return enabled;
}
