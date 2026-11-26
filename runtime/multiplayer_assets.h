#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "protocol.h"

#define SWMP_ARMOR_ACTOR_PREFIX "SwmpArmor"
#define SWMP_PROXY_ACTOR "SwmpRemoteLinkProxy"
#define SWMP_PROXY_PACK "Actor/Pack/" SWMP_PROXY_ACTOR ".sbactorpack"
#define SWMP_PROXY_TEMPLATE "Actor/Pack/Obj_RockGray_A_M_01.sbactorpack"
#define SWMP_ACTOR_INFO "Actor/ActorInfo.product.sbyml"
#define SWMP_PLAYER_ANIMATION "Player_Animation"
#define SWMP_PROXY_ANIMATION "SwmpLinkAnimation"
#define SWMP_PLAYER_ANIMATION_FILE "Model/" SWMP_PLAYER_ANIMATION ".sbfres"
#define SWMP_PROXY_ANIMATION_FILE "Model/" SWMP_PROXY_ANIMATION ".sbfres"

typedef struct {
    SwmpGearSlot slot;
    const char *model, *actor, *label;
    const char *template_pack, *bundle, *pack, *actor_link, *model_user, *model_list;
} SwmpGearAsset;

enum {
    SWMP_PARAGLIDER_ASSET = SWMP_GEAR_COUNT,
    SWMP_ARROW_ASSET_BASE = SWMP_PARAGLIDER_ASSET,
    SWMP_VISUAL_ASSET_COUNT = SWMP_ARROW_ASSET_BASE + SWMP_ARROW_COUNT,
    SWMP_PARAGLIDER_SLOT = SWMP_GEAR_SLOTS,
    SWMP_NOCKED_ARROW_SLOT,
    SWMP_FIRST_ARMOR_SLOT,
    SWMP_VISUAL_SLOTS = SWMP_FIRST_ARMOR_SLOT + SWMP_ARMOR_SLOTS,
};

enum { SWMP_MAX_ARMOR_MODELS = 160, SWMP_MAX_ARMOR_NAMES = 512 };

typedef struct {
    uint32_t id;
    SwmpArmorSlot slot;
    uint32_t heap_size;
    char actor[24], folder[32], model[48];
    bool ready;
} SwmpArmorAsset;

typedef struct {
    SwmpArmorAsset models[SWMP_MAX_ARMOR_MODELS];
    struct { uint32_t native_id, model_id; SwmpArmorSlot slot; } names[SWMP_MAX_ARMOR_NAMES];
    uint32_t defaults[SWMP_ARMOR_SLOTS];
    unsigned model_count, name_count;
} SwmpArmorCatalog;

extern const SwmpGearAsset swmp_gear_assets[SWMP_VISUAL_ASSET_COUNT];

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
    SWMP_PROXY_BOW_RELOAD,
    SWMP_PROXY_BOW_AIM,
    SWMP_PROXY_BOW_SHOOT,
    SWMP_PROXY_SWIM_WAIT,
    SWMP_PROXY_SWIM_MOVE,
    SWMP_PROXY_SWIM_DASH,
    SWMP_PROXY_CLIMB_WAIT,
    SWMP_PROXY_CLIMB_MOVE,
    SWMP_PROXY_CLIMB_JUMP,
    SWMP_PROXY_CLIMB_RIGHT,
    SWMP_PROXY_CLIMB_UP_RIGHT,
    SWMP_PROXY_CLIMB_UP_LEFT,
    SWMP_PROXY_CLIMB_LEFT,
    SWMP_PROXY_CLIMB_DOWN_LEFT,
    SWMP_PROXY_CLIMB_DOWN,
    SWMP_PROXY_CLIMB_DOWN_RIGHT,
    SWMP_PROXY_CLIMB_ENTER,
    SWMP_PROXY_CLIMB_MANTLE,
    SWMP_PROXY_GLIDE_DEPLOY,
    SWMP_PROXY_GLIDE,
    SWMP_PROXY_FALL,
    SWMP_PROXY_CLIMB_JUMP_RIGHT,
    SWMP_PROXY_CLIMB_JUMP_UP_RIGHT,
    SWMP_PROXY_CLIMB_JUMP_UP_LEFT,
    SWMP_PROXY_CLIMB_JUMP_LEFT,
    SWMP_PROXY_SWIM_FAST,
    SWMP_PROXY_SWIM_SLOW_LEFT,
    SWMP_PROXY_SWIM_SLOW_RIGHT,
    SWMP_PROXY_SWIM_FAST_LEFT,
    SWMP_PROXY_SWIM_FAST_RIGHT,
    SWMP_PROXY_SWIM_LOCK_FORWARD,
    SWMP_PROXY_SWIM_LOCK_BACK,
    SWMP_PROXY_SWIM_LOCK_LEFT,
    SWMP_PROXY_SWIM_LOCK_RIGHT,
    SWMP_PROXY_SWORD_DRAW,
    SWMP_PROXY_SWORD_READY,
    SWMP_PROXY_SWORD_STOW,
    SWMP_PROXY_SWORD_SWING,
    SWMP_PROXY_SWORD_CHAIN_2,
    SWMP_PROXY_SWORD_CHAIN_3,
    SWMP_PROXY_SWORD_FINISH,
    SWMP_PROXY_SPEAR_DRAW,
    SWMP_PROXY_SPEAR_READY,
    SWMP_PROXY_SPEAR_STOW,
    SWMP_PROXY_SPEAR_THRUST,
    SWMP_PROXY_SPEAR_CHAIN_2,
    SWMP_PROXY_SPEAR_FINISH,
    SWMP_PROXY_SPEAR_CHAIN_3,
    SWMP_PROXY_SPEAR_CHAIN_4,
    SWMP_PROXY_AXE_CHARGE_READY,
    SWMP_PROXY_SWORD_CHARGE_READY,
    SWMP_PROXY_SWORD_CHARGE,
    SWMP_PROXY_SPEAR_CHARGE_READY,
    SWMP_PROXY_SPEAR_CHARGE,
    SWMP_PROXY_SPEAR_CHARGE_FINISH,
    SWMP_PROXY_SHIELD_PARRY,
    SWMP_PROXY_SHIELD_HIT,
    SWMP_PROXY_SHIELD_BREAK,
    SWMP_PROXY_BOW_DRAW,
    SWMP_PROXY_BOW_STOW,
    SWMP_PROXY_BOW_READY,
    SWMP_PROXY_STEP,
    SWMP_PROXY_STEP_LEFT,
    SWMP_PROXY_STEP_RIGHT,
    SWMP_PROXY_STEP_FORWARD,
    SWMP_PROXY_STEP_BACK,
    SWMP_PROXY_STEP_LAND,
    SWMP_PROXY_STEP_LAND_LEFT,
    SWMP_PROXY_STEP_LAND_RIGHT,
    SWMP_PROXY_STEP_LAND_FORWARD,
    SWMP_PROXY_STEP_LAND_BACK,
    SWMP_PROXY_BACK_JUMP,
    SWMP_PROXY_BACK_JUMP_LAND,
    SWMP_PROXY_DAMAGE_LIGHT,
    SWMP_PROXY_DAMAGE_MEDIUM,
    SWMP_PROXY_KNOCKDOWN,
    SWMP_PROXY_GET_UP,
    SWMP_PROXY_TUMBLE,
    SWMP_PROXY_TUMBLE_LAND,
    SWMP_PROXY_HARD_LAND,
    SWMP_PROXY_HARD_LAND_RECOVER,
    SWMP_PROXY_ACTION_COUNT,
} SwmpProxyAction;

const char *swmp_proxy_action_name(SwmpProxyAction action);
SwmpProxyAction swmp_proxy_sample_action(const char *state, const char *upper, SwmpGearSet gear,
                                         const SwmpGearPose *weapon_pose, const SwmpGearPose *bow_pose,
                                         bool shield_raised);
// Build uncompressed RomFS resources from the local game's data. The caller owns the result.
uint8_t *swmp_proxy_pack(const uint8_t *src, size_t size, bool in_place, size_t *out_size);
uint8_t *swmp_proxy_animation(const uint8_t *src, size_t size, size_t *out_size);
uint8_t *swmp_gear_pack(const SwmpGearAsset *asset, const uint8_t *src, size_t size,
                        const uint8_t *gear, size_t gear_size, size_t *out_size);
bool swmp_armor_catalog(const uint8_t *src, size_t size, SwmpArmorCatalog *out);
uint32_t swmp_armor_identity(const SwmpArmorCatalog *catalog, SwmpArmorSlot slot, const char *native_name);
uint8_t *swmp_armor_pack(const SwmpArmorAsset *asset, const uint8_t *src, size_t size,
                         bool in_place, size_t *out_size);
uint8_t *swmp_proxy_actor_info(const uint8_t *src, size_t size, unsigned gear_mask,
                              const SwmpArmorCatalog *armor, size_t *out_size);
bool fs_multiplayer_proxy_ready(void);
bool fs_multiplayer_gear_ready(unsigned asset);
const SwmpArmorCatalog *fs_multiplayer_armor_catalog(void);
