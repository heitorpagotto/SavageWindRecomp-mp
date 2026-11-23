#pragma once

#include "../multiplayer.h"

#ifdef SW_MULTIPLAYER
void multiplayer_tab_draw();
void multiplayer_notices_draw(float scale);
const char *multiplayer_status(const SwmpM1View &view);
#endif
