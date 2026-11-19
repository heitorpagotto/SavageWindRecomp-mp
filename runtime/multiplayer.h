// Built-in M1 developer lobby. The main window thread owns transport and publishes copies to the menu.
#pragma once

#ifdef SWMP_M1_TEST

#include "gamelib.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "lobby.h"

typedef enum {
    SWMP_M1_HOST = 1,
    SWMP_M1_JOIN,
    SWMP_M1_LEAVE,
    SWMP_M1_START,
} SwmpM1Action;

typedef struct {
    bool enabled, configured, joining, failed;
    SwmpSession session;
    SwmpRejectCode rejection;
    char invite[SWMP_INVITE_CODE_SIZE];
} SwmpM1View;

typedef struct {
    uint64_t lobby_generation;  // zero when there is no admitted session
    SwmpRemoteRoster remotes;
    uint8_t local_peer;
} SwmpM1Roster;

void swmp_m1_boot(const GameLibTables *game);
void swmp_m1_update(void);
void swmp_m1_shutdown(void);
SwmpM1View swmp_m1_view(void);
// Game-thread reader. No guest references or transport state cross this boundary.
SwmpM1Roster swmp_m1_roster(void);
void swmp_m1_submit(SwmpM1Action action, const char *name, const char *invite);

#ifdef __cplusplus
}
#endif
#endif
