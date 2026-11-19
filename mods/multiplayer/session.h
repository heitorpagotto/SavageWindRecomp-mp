// Host-authoritative session bookkeeping. All functions are called from the multiplayer game thread.
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "protocol.h"

#define SWMP_PLAYER_NAME_SIZE 32u

typedef enum {
    SWMP_ROLE_NONE = 0,
    SWMP_ROLE_HOST,
    SWMP_ROLE_CLIENT
} SwmpSessionRole;

typedef enum {
    SWMP_SESSION_OFFLINE = 0,
    SWMP_SESSION_CONNECTING,
    SWMP_SESSION_LOBBY,
    SWMP_SESSION_PLAYING
} SwmpSessionState;

typedef enum {
    SWMP_PEER_EMPTY = 0,
    SWMP_PEER_CONNECTING,
    SWMP_PEER_READY
} SwmpPeerState;

typedef struct {
    SwmpPeerState state;
    uint8_t id;
    uint8_t sequence_seen;  // one bit per SwmpChannel
    uint32_t last_sequence[SWMP_CHANNEL_COUNT];
    char name[SWMP_PLAYER_NAME_SIZE];
} SwmpPeer;

typedef struct {
    SwmpSessionRole role;
    SwmpSessionState state;
    uint64_t session_id;
    uint8_t local_peer;
    uint8_t host_peer;
    uint32_t next_sequence[SWMP_CHANNEL_COUNT];
    char local_name[SWMP_PLAYER_NAME_SIZE];
    SwmpPeer peers[SWMP_MAX_PEERS];
} SwmpSession;

typedef enum {
    SWMP_RECEIVE_ACCEPT = 0,
    SWMP_RECEIVE_DROP_STATE,
    SWMP_RECEIVE_DROP_SESSION,
    SWMP_RECEIVE_DROP_PEER,
    SWMP_RECEIVE_DROP_STALE
} SwmpReceiveDecision;

void swmp_session_init(SwmpSession *session);
bool swmp_session_host(SwmpSession *session, uint64_t session_id, const char *name);
bool swmp_session_join(SwmpSession *session, uint64_t session_id, const char *name);
bool swmp_session_client_welcome(SwmpSession *session, uint8_t local_peer, const char *host_name);
bool swmp_session_add_remote_peer(SwmpSession *session, uint8_t peer_id, const char *name);

// The host assigns the first free id. The caller has already authenticated and checked protocol/game compatibility.
// Returns -1 when the session is not hosting or all eight slots are occupied.
int swmp_session_add_peer(SwmpSession *session, const char *name);
bool swmp_session_remove_peer(SwmpSession *session, uint8_t peer_id);
const SwmpPeer *swmp_session_peer(const SwmpSession *session, uint8_t peer_id);
size_t swmp_session_peer_count(const SwmpSession *session);

bool swmp_session_start(SwmpSession *session);         // host starts gameplay
bool swmp_session_remote_start(SwmpSession *session);  // client receives START
bool swmp_session_next_sequence(SwmpSession *session, SwmpChannel channel, uint32_t *sequence);
SwmpReceiveDecision swmp_session_accept(const SwmpSession *session, const SwmpPacketHeader *header);
SwmpReceiveDecision swmp_session_receive(SwmpSession *session, const SwmpPacketHeader *header);

