#pragma once
#include <stdbool.h>
#include "cpu.h"

// Layout ownership travels with deferred NVN commands, rather than the live event manager.
enum { UI_LAYOUT_NONE, UI_LAYOUT_HUD, UI_LAYOUT_DIALOGUE, UI_LAYOUT_CONTEXT, UI_LAYOUT_MENU, UI_LAYOUT_TALK };
// -1 unsupported/uninitialized; 0 minimap; 1 full map (including transitions).
int ui_layout_map_mode(uint64_t screen);
// The full overworld map's pan (layout units) and zoom (layout units per world metre).
bool ui_layout_world_map_view(float view[3]);
uint64_t ui_layout_begin(Cpu *c);
void ui_layout_end(Cpu *c, uint64_t commands);
void rt_ui_layout_draw(Cpu *c, void (*native)(Cpu *));
bool rt_ui_layout_pane_draw(Cpu *c, void (*native)(Cpu *));
