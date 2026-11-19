// Host-authoritative control plane. Poll with a caller-provided monotonic clock in milliseconds.
#pragma once

#include "session.h"
#include "transport.h"

#define SWMP_LOBBY_TIMEOUT_MS UINT64_C(5000)
#define SWMP_LOBBY_LINKS (2u * SWMP_MAX_PEERS)

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
    uint32_t features;  // client: negotiated features; host: supported features
    bool joining;       // session ID is unknown until WELCOME
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
// Consumes control and lifecycle events. Gameplay channel dispatch is reserved for later integration.
void swmp_lobby_poll(SwmpLobby *lobby, uint64_t now_ms);
bool swmp_lobby_start(SwmpLobby *lobby);
void swmp_lobby_leave(SwmpLobby *lobby);
