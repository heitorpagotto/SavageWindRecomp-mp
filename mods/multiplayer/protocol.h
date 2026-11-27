// Savage Wind Multiplayer wire protocol. This core is independent of the game runtime and contains no game data.
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SWMP_PROTOCOL_VERSION 9u
#define SWMP_MAX_PEERS 8u
#define SWMP_PLAYER_NAME_SIZE 32u
#define SWMP_HOST_PEER 0u
#define SWMP_UNASSIGNED_PEER 0xffu
#define SWMP_PACKET_HEADER_SIZE 32u
#define SWMP_PLAYER_SNAPSHOT_SIZE 101u
#define SWMP_GEAR_OFFSET_LIMIT_MM 3000
#define SWMP_ENVIRONMENT_SIZE 8u
#define SWMP_DAY_MILLI 360000u  // TimeMgr uses 360 units per day.
#define SWMP_WEATHER_TYPES 9u
#define SWMP_WEATHER_UNAVAILABLE 255u
#define SWMP_MAX_PAYLOAD_SIZE (64u * 1024u)

typedef enum {
    SWMP_CHANNEL_CONTROL = 0,  // reliable, ordered: connection and lobby messages
    SWMP_CHANNEL_STATE = 1,    // unreliable, unordered: latest player state wins
    SWMP_CHANNEL_EVENT = 2,    // reliable, ordered: gameplay transactions
    SWMP_CHANNEL_COUNT
} SwmpChannel;

typedef enum {
    SWMP_MSG_HELLO = 1,
    SWMP_MSG_WELCOME = 2,
    SWMP_MSG_REJECT = 3,
    SWMP_MSG_PEER_JOINED = 4,
    SWMP_MSG_PEER_LEFT = 5,
    SWMP_MSG_START = 6,
    SWMP_MSG_PING = 7,
    SWMP_MSG_PONG = 8,

    SWMP_MSG_PLAYER_SNAPSHOT = 16,
    SWMP_MSG_EQUIPMENT = 17,

    SWMP_MSG_DAMAGE_REQUEST = 32,
    SWMP_MSG_DAMAGE_APPLIED = 33,
    SWMP_MSG_ACTOR_HEALTH = 34,

    SWMP_MSG_FLAG_TRANSACTION = 48,
    SWMP_MSG_CHEST_REWARD = 49,
    SWMP_MSG_WORLD_PHASE = 50,

    SWMP_MSG_RESYNC_REQUEST = 64,
    SWMP_MSG_RESYNC_SNAPSHOT = 65
} SwmpMessageType;

enum {
    SWMP_FLAG_REQUEST = 1u << 0,
    SWMP_FLAG_RESPONSE = 1u << 1,
    SWMP_FLAG_CRITICAL = 1u << 2,
    SWMP_FLAG_KNOWN = SWMP_FLAG_REQUEST | SWMP_FLAG_RESPONSE | SWMP_FLAG_CRITICAL
};

typedef struct {
    uint16_t version;
    uint8_t type;
    uint8_t flags;
    uint32_t payload_size;
    uint64_t session_id;  // identifies a session; transport encryption provides secrecy
    uint8_t sender_peer;
    uint8_t channel;
    uint32_t sequence;
    uint32_t tick;        // message-specific tick (host membership epoch for lobby position traffic)
} SwmpPacketHeader;

typedef enum {
    SWMP_PACKET_OK = 0,
    SWMP_PACKET_BUFFER_TOO_SMALL,
    SWMP_PACKET_BAD_MAGIC,
    SWMP_PACKET_BAD_VERSION,
    SWMP_PACKET_BAD_TYPE,
    SWMP_PACKET_BAD_FLAGS,
    SWMP_PACKET_BAD_CHANNEL,
    SWMP_PACKET_BAD_PEER,
    SWMP_PACKET_BAD_RESERVED,
    SWMP_PACKET_BAD_PAYLOAD_SIZE,
    SWMP_PACKET_SIZE_MISMATCH
} SwmpPacketError;

// Integers are encoded in network byte order. The output buffer includes the payload after the header; this function
// writes only the header but checks that the entire declared packet fits.
SwmpPacketError swmp_packet_encode(uint8_t *out, size_t out_size, const SwmpPacketHeader *header);
SwmpPacketError swmp_packet_decode(SwmpPacketHeader *header, const uint8_t *packet, size_t packet_size);
const char *swmp_packet_error_string(SwmpPacketError error);

enum {
    SWMP_PLAYER_GROUNDED = 1u << 0,
    SWMP_PLAYER_GLIDING = 1u << 1,
    SWMP_PLAYER_CROUCHING = 1u << 2,
    SWMP_PLAYER_GUARDING = 1u << 3,
    SWMP_PLAYER_DEAD = 1u << 4,
    SWMP_PLAYER_WEAPON_POSE = 1u << 5,
    SWMP_PLAYER_SHIELD_POSE = 1u << 6,
    SWMP_PLAYER_BOW_POSE = 1u << 7,
    SWMP_PLAYER_STATE_KNOWN = SWMP_PLAYER_GROUNDED | SWMP_PLAYER_GLIDING | SWMP_PLAYER_CROUCHING |
                              SWMP_PLAYER_GUARDING | SWMP_PLAYER_DEAD |
                              SWMP_PLAYER_WEAPON_POSE | SWMP_PLAYER_SHIELD_POSE | SWMP_PLAYER_BOW_POSE
};

typedef enum {
    SWMP_GEAR_WEAPON,
    SWMP_GEAR_SHIELD,
    SWMP_GEAR_BOW,
    SWMP_GEAR_SLOTS
} SwmpGearSlot;

typedef enum {
    SWMP_GEAR_FAMILY_NONE,
    SWMP_GEAR_FAMILY_SWORD,
    SWMP_GEAR_FAMILY_LSWORD,
    SWMP_GEAR_FAMILY_SPEAR,
    SWMP_GEAR_FAMILY_SHIELD,
    SWMP_GEAR_FAMILY_BOW,
    SWMP_GEAR_FAMILY_COUNT
} SwmpGearFamily;

enum { SWMP_GEAR_NONE = 0, SWMP_GEAR_VARIANTS = 1000 };

// Match the numeric suffix of a stock Weapon_<family>_NNN actor.
#define SWMP_GEAR_ID(family, variant) (SWMP_GEAR_FAMILY_##family * SWMP_GEAR_VARIANTS + (variant))

static inline SwmpGearFamily swmp_gear_family(uint16_t id) {
    unsigned family = id / SWMP_GEAR_VARIANTS;
    return id % SWMP_GEAR_VARIANTS && family < SWMP_GEAR_FAMILY_COUNT ?
        (SwmpGearFamily)family : SWMP_GEAR_FAMILY_NONE;
}

typedef enum {
    SWMP_ARMOR_HEAD,
    SWMP_ARMOR_UPPER,
    SWMP_ARMOR_LOWER,
    SWMP_ARMOR_SLOTS
} SwmpArmorSlot;

enum { SWMP_ARMOR_DYE_MAX = 15 };

typedef struct {
    uint16_t id[SWMP_GEAR_SLOTS];  // family and numeric variant, resolved against installed resources
    uint32_t armor[SWMP_ARMOR_SLOTS];  // installed model identities, zero for the default appearance
    uint8_t armor_dye[SWMP_ARMOR_SLOTS];  // zero selects the native undyed palette
} SwmpGearSet;

typedef struct {
    uint32_t revision;  // zero until the owner advertises its first equipment change
    SwmpGearSet gear;
} SwmpEquipment;

typedef struct {
    int16_t offset_mm[3];  // relative to Link's yaw frame
    int16_t rotation[3];  // relative pitch, yaw, roll as signed turn fractions
} SwmpGearPose;

typedef enum {
    SWMP_ARROW_NONE,
    SWMP_ARROW_NORMAL,
    SWMP_ARROW_FIRE,
    SWMP_ARROW_ICE,
    SWMP_ARROW_SHOCK,
    SWMP_ARROW_BOMB,
    SWMP_ARROW_ANCIENT,
    SWMP_ARROW_COUNT,
} SwmpArrow;

typedef enum {
    SWMP_BOW_ANIM_NONE,
    SWMP_BOW_ANIM_HOLD,
    SWMP_BOW_ANIM_EQUIPPED,
    SWMP_BOW_ANIM_DRAW,
    SWMP_BOW_ANIM_SHOOT,
    SWMP_BOW_ANIM_NEUTRAL,
    SWMP_BOW_ANIM_CAMERA_PAUSE,
    SWMP_BOW_ANIM_COUNT,
} SwmpBowAnimation;

typedef enum {
    SWMP_SHIELD_ANIM_NONE,
    SWMP_SHIELD_ANIM_OFF,
    SWMP_SHIELD_ANIM_ON,
    SWMP_SHIELD_ANIM_EQUIP_ON,
    SWMP_SHIELD_ANIM_EQUIP_OFF,
    SWMP_SHIELD_ANIM_TRANSFORM,
    SWMP_SHIELD_ANIM_FOLD,
    SWMP_SHIELD_ANIM_COUNT,
} SwmpShieldAnimation;

static inline unsigned swmp_gear_pose_flag(SwmpGearSlot slot) {
    return SWMP_PLAYER_WEAPON_POSE << slot;
}

bool swmp_gear_set_valid(SwmpGearSet gear);

// A compact, renderer-independent state sample. Area and phase identify which world instance can see the sample.
// Equipment is sent separately and referenced by a monotonically increasing revision.
typedef struct {
    uint8_t peer_id;
    uint8_t flags;
    uint16_t action;
    uint32_t area_id;
    uint32_t phase_id;
    int32_t position_mm[3];
    int16_t rotation[3];       // pitch, yaw, roll as signed turn fractions
    int16_t velocity_cm_s[3];
    uint16_t health_q4;
    uint16_t stamina_q4;
    uint32_t equipment_revision;
    SwmpGearPose gear_pose[SWMP_GEAR_SLOTS];
    uint8_t nocked_arrow;  // SwmpArrow, valid with a bow pose
    uint8_t bow_animation; // SwmpBowAnimation, valid with a bow pose
    uint8_t shield_animation; // SwmpShieldAnimation, valid with a shield pose
    uint8_t animation_activation; // nonzero for a tracked clip instance
    uint8_t animation_phase;      // 1..255 maps to [0, 1], zero if unavailable
    SwmpGearPose arrow_pose;
    uint32_t sampled_ms;          // low 32 bits of the owning game's monotonic sample time
} SwmpPlayerSnapshot;

bool swmp_player_snapshot_encode(uint8_t out[SWMP_PLAYER_SNAPSHOT_SIZE], const SwmpPlayerSnapshot *snapshot);
bool swmp_player_snapshot_decode(SwmpPlayerSnapshot *snapshot,
                                 const uint8_t data[SWMP_PLAYER_SNAPSHOT_SIZE]);

// Optional prefix on a host position batch. Its area comes from the host's pose in the same batch.
typedef struct {
    uint32_t time_milli;
    uint8_t weather;
} SwmpEnvironment;

bool swmp_environment_encode(uint8_t out[SWMP_ENVIRONMENT_SIZE], const SwmpEnvironment *environment);
bool swmp_environment_decode(SwmpEnvironment *environment, const uint8_t data[SWMP_ENVIRONMENT_SIZE]);

