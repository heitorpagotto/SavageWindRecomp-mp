// Called by the marker test with an overlay path and the expected enable state.
#include "package_marker.h"

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc != 3 || (strcmp(argv[2], "enabled") && strcmp(argv[2], "disabled"))) return 2;
    bool actual = swmp_package_marker_enabled(argv[1]);
    bool expected = !strcmp(argv[2], "enabled");
    if (actual == expected) return 0;
    fprintf(stderr, "Unexpected multiplayer marker state for %s\n", argv[1]);
    return 1;
}
