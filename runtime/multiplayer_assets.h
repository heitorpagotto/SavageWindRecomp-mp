#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SWMP_PROXY_ACTOR "SwmpRemoteLinkProxy"
#define SWMP_PROXY_PACK "Actor/Pack/" SWMP_PROXY_ACTOR ".sbactorpack"
#define SWMP_PROXY_TEMPLATE "Actor/Pack/Obj_RockGray_A_M_01.sbactorpack"
#define SWMP_ACTOR_INFO "Actor/ActorInfo.product.sbyml"

// Build uncompressed RomFS resources from the local game's data. The caller owns the result.
uint8_t *swmp_proxy_pack(const uint8_t *src, size_t size, size_t *out_size);
uint8_t *swmp_proxy_actor_info(const uint8_t *src, size_t size, size_t *out_size);
bool fs_multiplayer_proxy_ready(void);
