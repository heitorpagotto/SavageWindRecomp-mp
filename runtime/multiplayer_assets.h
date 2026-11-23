#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SWMP_PROXY_ACTOR "SwmpRemoteLinkProxy"
#define SWMP_PROXY_PACK "Actor/Pack/" SWMP_PROXY_ACTOR ".sbactorpack"
#define SWMP_PROXY_TEMPLATE "Actor/Pack/Obj_RockGray_A_M_01.sbactorpack"
#define SWMP_ACTOR_INFO "Actor/ActorInfo.product.sbyml"

// Stable values carried in the player snapshot's action field.
typedef enum {
    SWMP_PROXY_IDLE,
    SWMP_PROXY_JUMP,
    SWMP_PROXY_WALK,
    SWMP_PROXY_DASH,
    SWMP_PROXY_RUN,
    SWMP_PROXY_ACTION_COUNT,
} SwmpProxyAction;

const char *swmp_proxy_action_name(SwmpProxyAction action);
// Build uncompressed RomFS resources from the local game's data. The caller owns the result.
uint8_t *swmp_proxy_pack(const uint8_t *src, size_t size, size_t *out_size);
uint8_t *swmp_proxy_actor_info(const uint8_t *src, size_t size, size_t *out_size);
bool fs_multiplayer_proxy_ready(void);
