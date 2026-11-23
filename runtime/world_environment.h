#pragma once

#include <stdbool.h>
#include <stdint.h>

// TimeMgr's clock, 0..360 units per day. Writes also update the live WM_Time buffers.
bool world_time_read(float *tod);
bool world_time_sync(float tod);

// The world manager uses 0..8 for weather and 255 when no type is available.
struct Cpu;
uint8_t world_weather_read(const struct Cpu *cpu);
void world_weather_sync(uint8_t type);
