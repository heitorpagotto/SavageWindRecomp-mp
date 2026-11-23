// Savage Wind Multiplayer wire protocol. This core is independent of the game runtime and contains no game data.
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SWMP_PROTOCOL_VERSION 1u
#define SWMP_MAX_PEERS 8u
#define SWMP_PLAYER_NAME_SIZE 32u
#define SWMP_HOST_PEER 0u
#define SWMP_UNASSIGNED_PEER 0xffu
#define SWMP_PACKET_HEADER_SIZE 32u
#define SWMP_PLAYER_SNAPSHOT_SIZE 44u
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
    SWMP_PLAYER_STATE_KNOWN = SWMP_PLAYER_GROUNDED | SWMP_PLAYER_GLIDING | SWMP_PLAYER_CROUCHING |
                              SWMP_PLAYER_GUARDING | SWMP_PLAYER_DEAD
};

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

