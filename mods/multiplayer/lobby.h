// Host-authoritative control plane. Poll with a caller-provided monotonic clock in milliseconds.
#pragma once

#include "session.h"
#include "transport.h"

#define SWMP_LOBBY_TIMEOUT_MS UINT64_C(5000)
#define SWMP_LOBBY_LINKS (2u * SWMP_MAX_PEERS)
#define SWMP_POSITION_INTERVAL_MS UINT64_C(50)
#define SWMP_POSITION_TIMEOUT_MS UINT64_C(1000)

typedef struct {
    SwmpPlayerSnapshot snapshot;
    uint64_t received_at;
} SwmpPosition;

typedef struct {
    SwmpEnvironment state;
    uint32_t area_id;  // zero when unavailable
    uint64_t received_at;
} SwmpHostEnvironment;

typedef struct {
    SwmpConnection connection;
    uint64_t connected_at;
    uint8_t binding[SWMP_BINDING_SIZE];
    uint8_t peer;  // unassigned until HELLO/WELCOME completes
    bool closing;
} SwmpLobbyLink;

typedef struct {
    SwmpTransport transport;
    SwmpSession session;
    SwmpCompatibility compatibility;
    SwmpLobbyLink links[SWMP_LOBBY_LINKS];
    uint64_t join_started_at;
    uint64_t next_position_at;
    uint32_t roster_epoch;  // host version of the reliable membership list
    SwmpPosition positions[SWMP_MAX_PEERS];
    SwmpEquipment equipment[SWMP_MAX_PEERS];
    SwmpHostEnvironment environment;
    void (*peer_changed)(const char *name, bool joined, uint64_t now_ms);
    uint32_t features;  // client: negotiated features; host: supported features
    bool joining;       // session ID is unknown until WELCOME
    bool waiting_welcome;
    char name[SWMP_PLAYER_NAME_SIZE];
    char invite[SWMP_INVITE_CODE_SIZE];
    SwmpRejectCode rejection;
    char error[SWMP_TRANSPORT_ERROR_SIZE];
} SwmpLobby;

void swmp_lobby_init(SwmpLobby *lobby, SwmpTransport transport);
bool swmp_lobby_host(SwmpLobby *lobby, const SwmpCompatibility *compatibility, const char *name,
                    const char *url, uint64_t session_id);
bool swmp_lobby_join(SwmpLobby *lobby, const SwmpCompatibility *compatibility, const char *name,
                    const char *url, const char *invite, uint64_t now_ms);
void swmp_lobby_set_equipment(SwmpLobby *lobby, SwmpGearSet gear);
// A sample with area_id zero withdraws local presence. Only the host can supply an environment.
void swmp_lobby_send_state(SwmpLobby *lobby, const SwmpPlayerSnapshot *local,
                           const SwmpEnvironment *environment, uint64_t now_ms);
void swmp_lobby_send_position(SwmpLobby *lobby, const SwmpPlayerSnapshot *local, uint64_t now_ms);
bool swmp_position_fresh(const SwmpPosition *position, uint64_t now_ms);
// Consumes control, position and lifecycle events.
void swmp_lobby_poll(SwmpLobby *lobby, uint64_t now_ms);
bool swmp_lobby_start(SwmpLobby *lobby);
void swmp_lobby_leave(SwmpLobby *lobby);
