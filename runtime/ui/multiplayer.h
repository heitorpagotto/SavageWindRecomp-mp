#pragma once

#include "../multiplayer.h"

#ifdef SW_MULTIPLAYER
void multiplayer_tab_reset();
void multiplayer_tab_draw();
const char *multiplayer_status(const SwmpM1View &view);
#endif
