// Stock SWMM deploys the marker as RomFS data, sometimes compressed with Yaz0.
#pragma once

#include <stdbool.h>

// The marker is read only from the selected overlay.
bool swmp_package_marker_enabled(const char *overlay);
