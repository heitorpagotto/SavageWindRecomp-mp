// The window thread owns transport and publishes copies to the menus and guest update.
#pragma once

#ifdef SW_MULTIPLAYER

#include "gamelib.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "nvn/output_scale.h"
#include "lobby.h"
#include "direct.h"

typedef enum {
    SWMP_M1_HOST = 1,
    SWMP_M1_JOIN,
    SWMP_M1_LEAVE,
    SWMP_M1_START,
    SWMP_M1_NEW_INVITE,
    SWMP_M1_COPY,
    SWMP_M1_PASTE_REPLY,
} SwmpM1Action;

typedef struct {
    bool enabled, supported, pending, joining, awaiting_peer, failed, clipboard_error;
    SwmpDirectState direct;
    SwmpSession session;
    SwmpRejectCode rejection;
    // Present only while the peer's snapshot is fresh.
    uint16_t health_q4[SWMP_MAX_PEERS];
    uint8_t health_known;
} SwmpM1View;

typedef struct {
    uint64_t lobby_generation;  // zero when there is no admitted session
    SwmpRemoteRoster remotes;
    SwmpPosition positions[SWMP_MAX_PEERS];
    SwmpEquipment equipment[SWMP_MAX_PEERS];
    SwmpHostEnvironment environment;
    bool host;
} SwmpM1Roster;

typedef enum {
    SWMP_NOTICE_JOINED,
    SWMP_NOTICE_LEFT,
    SWMP_NOTICE_LOCAL_LEFT,
} SwmpNoticeType;

typedef struct {
    SwmpNoticeType type;
    char name[SWMP_PLAYER_NAME_SIZE];
} SwmpNotice;

typedef struct {
    uint64_t generation;
    float above_head[3];
} SwmpNameAnchor;

typedef struct SwmpIndicatorOverlay {
    char names[SWMP_MAX_PEERS][SWMP_PLAYER_NAME_SIZE];
    float points[SWMP_MAX_PEERS][2], clip[4];
    uint8_t visible;
    bool map;
} SwmpIndicatorOverlay;

void swmp_m1_boot(const GameLibTables *game);
void swmp_m1_update(void);
void swmp_m1_shutdown(void);
void swmp_m1_set_enabled(bool enabled);
SwmpM1View swmp_m1_view(void);
// Game-thread reader. No guest references or transport state cross this boundary.
SwmpM1Roster swmp_m1_roster(void);
void swmp_m1_publish_state(uint64_t generation, const SwmpPlayerSnapshot *position,
                           SwmpGearSet gear, const SwmpEnvironment *environment, uint64_t now_ms);
void swmp_m1_publish_name_anchors(uint64_t generation, const SwmpNameAnchor anchors[SWMP_MAX_PEERS]);
SwmpIndicatorOverlay swmp_m1_indicator_overlay(const OutScale *output, const float world_to_clip[16]);
bool swmp_m1_weather_active(void);
// A null output checks whether the overlay needs to render.
size_t swmp_m1_notices(SwmpNotice notices[SWMP_MAX_PEERS]);
bool swmp_m1_can(const SwmpM1View *view, SwmpM1Action action);
void swmp_m1_submit(SwmpM1Action action, const char *name);

#ifdef __cplusplus
}
#endif
#endif
