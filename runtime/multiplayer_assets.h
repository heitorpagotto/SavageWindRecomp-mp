#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "protocol.h"

#define SWMP_PROXY_ACTOR "SwmpRemoteLinkProxy"
#define SWMP_PROXY_PACK "Actor/Pack/" SWMP_PROXY_ACTOR ".sbactorpack"
#define SWMP_PROXY_TEMPLATE "Actor/Pack/Obj_RockGray_A_M_01.sbactorpack"
#define SWMP_ACTOR_INFO "Actor/ActorInfo.product.sbyml"

typedef struct {
    SwmpGearSlot slot;
    const char *model, *actor, *label;
    const char *template_pack, *pack, *actor_link, *model_list;
} SwmpGearAsset;

extern const SwmpGearAsset swmp_gear_assets[SWMP_GEAR_COUNT];

// Stable values carried in the player snapshot's action field.
typedef enum {
    SWMP_PROXY_IDLE,
    SWMP_PROXY_JUMP,
    SWMP_PROXY_WALK,
    SWMP_PROXY_DASH,
    SWMP_PROXY_RUN,
    SWMP_PROXY_LAND,
    SWMP_PROXY_AXE_DRAW,
    SWMP_PROXY_AXE_SWING,
    SWMP_PROXY_AXE_FINISH,
    SWMP_PROXY_AXE_SPIN,
    SWMP_PROXY_AXE_READY,
    SWMP_PROXY_AXE_STOW,
    SWMP_PROXY_CROUCH_IDLE,
    SWMP_PROXY_CROUCH_MOVE,
    SWMP_PROXY_SHIELD_GUARD,
    SWMP_PROXY_ACTION_COUNT,
} SwmpProxyAction;

const char *swmp_proxy_action_name(SwmpProxyAction action);
SwmpProxyAction swmp_proxy_sample_action(const char *state, SwmpGear weapon, bool drawn, bool shield_raised);
// Build uncompressed RomFS resources from the local game's data. The caller owns the result.
uint8_t *swmp_proxy_pack(const uint8_t *src, size_t size, size_t *out_size);
uint8_t *swmp_gear_pack(const SwmpGearAsset *asset, const uint8_t *src, size_t size,
                        const uint8_t *gear, size_t gear_size, size_t *out_size);
uint8_t *swmp_proxy_actor_info(const uint8_t *src, size_t size, unsigned gear_mask, size_t *out_size);
bool fs_multiplayer_proxy_ready(void);
bool fs_multiplayer_gear_ready(SwmpGear gear);
