// Wire payloads for the reliable control channel.
#pragma once

#include "protocol.h"

#define SWMP_REJECT_TEXT_SIZE 96u
#define SWMP_BINDING_SIZE 16u
#define SWMP_CONTROL_MAX_SIZE (6u + SWMP_MAX_PEERS * (1u + SWMP_PLAYER_NAME_SIZE))

typedef struct {
    uint16_t protocol;
    uint16_t mod_version[3];
    uint16_t game_version[3];
    uint8_t savage_build[20];
    uint8_t game_build[32];
    uint32_t features;
    uint32_t required_features;
} SwmpCompatibility;

typedef enum {
    SWMP_REJECT_NONE = 0,
    SWMP_REJECT_PROTOCOL,
    SWMP_REJECT_BUILD,
    SWMP_REJECT_FEATURES,
    SWMP_REJECT_FULL,
    SWMP_REJECT_INVALID
} SwmpRejectCode;

typedef union {
    struct {
        SwmpCompatibility compatibility;
        uint8_t binding[SWMP_BINDING_SIZE];
        char name[SWMP_PLAYER_NAME_SIZE];
    } hello;
    struct {
        uint8_t local_peer;
        bool playing;
        uint32_t features;  // supported feature intersection for this connection
        char names[SWMP_MAX_PEERS][SWMP_PLAYER_NAME_SIZE];  // empty entries are unoccupied
    } welcome;
    struct {
        SwmpRejectCode code;
        char text[SWMP_REJECT_TEXT_SIZE];
    } reject;
    struct {
        uint8_t id;
        char name[SWMP_PLAYER_NAME_SIZE];
    } peer;
    struct {
        uint8_t peer;
        SwmpEquipment state;
    } equipment;
    uint64_t ping_token;
} SwmpControl;

bool swmp_player_name_valid(const char *name);
bool swmp_control_encode(uint8_t *out, size_t capacity, size_t *size, SwmpMessageType type, const SwmpControl *msg);
bool swmp_control_decode(SwmpControl *msg, SwmpMessageType type, const uint8_t *data, size_t size);
