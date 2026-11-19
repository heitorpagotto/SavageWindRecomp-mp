#include "session.h"

#include <string.h>

static void copy_name(char out[SWMP_PLAYER_NAME_SIZE], const char *name) {
    if (!name) name = "";
    size_t n = strlen(name);
    if (n >= SWMP_PLAYER_NAME_SIZE) n = SWMP_PLAYER_NAME_SIZE - 1;
    memcpy(out, name, n);
    out[n] = 0;
}

void swmp_session_init(SwmpSession *s) {
    if (!s) return;
    memset(s, 0, sizeof *s);
    s->local_peer = SWMP_UNASSIGNED_PEER;
    s->host_peer = SWMP_UNASSIGNED_PEER;
    for (uint8_t i = 0; i < SWMP_MAX_PEERS; i++) s->peers[i].id = i;
    for (int i = 0; i < SWMP_CHANNEL_COUNT; i++) s->next_sequence[i] = 1;
}

bool swmp_session_host(SwmpSession *s, uint64_t session_id, const char *name) {
    if (!s || !session_id || s->state != SWMP_SESSION_OFFLINE) return false;
    swmp_session_init(s);
    s->role = SWMP_ROLE_HOST;
    s->state = SWMP_SESSION_LOBBY;
    s->session_id = session_id;
    s->local_peer = s->host_peer = SWMP_HOST_PEER;
    copy_name(s->local_name, name);
    s->peers[SWMP_HOST_PEER].state = SWMP_PEER_READY;
    copy_name(s->peers[SWMP_HOST_PEER].name, name);
    return true;
}

bool swmp_session_join(SwmpSession *s, uint64_t session_id, const char *name) {
    if (!s || !session_id || s->state != SWMP_SESSION_OFFLINE) return false;
    swmp_session_init(s);
    s->role = SWMP_ROLE_CLIENT;
    s->state = SWMP_SESSION_CONNECTING;
    s->session_id = session_id;
    s->host_peer = SWMP_HOST_PEER;
    copy_name(s->local_name, name);
    s->peers[SWMP_HOST_PEER].state = SWMP_PEER_CONNECTING;
    return true;
}

bool swmp_session_client_welcome(SwmpSession *s, uint8_t local_peer, const char *host_name) {
    if (!s || s->role != SWMP_ROLE_CLIENT || s->state != SWMP_SESSION_CONNECTING ||
        local_peer == SWMP_HOST_PEER || local_peer >= SWMP_MAX_PEERS)
        return false;
    s->local_peer = local_peer;
    s->state = SWMP_SESSION_LOBBY;
    s->peers[SWMP_HOST_PEER].state = SWMP_PEER_READY;
    copy_name(s->peers[SWMP_HOST_PEER].name, host_name);
    s->peers[local_peer].state = SWMP_PEER_READY;
    copy_name(s->peers[local_peer].name, s->local_name);
    return true;
}

bool swmp_session_add_remote_peer(SwmpSession *s, uint8_t peer_id, const char *name) {
    if (!s || s->role != SWMP_ROLE_CLIENT ||
        (s->state != SWMP_SESSION_LOBBY && s->state != SWMP_SESSION_PLAYING) ||
        peer_id >= SWMP_MAX_PEERS || peer_id == s->host_peer || peer_id == s->local_peer ||
        s->peers[peer_id].state != SWMP_PEER_EMPTY)
        return false;
    s->peers[peer_id].state = SWMP_PEER_READY;
    copy_name(s->peers[peer_id].name, name);
    return true;
}

int swmp_session_add_peer(SwmpSession *s, const char *name) {
    if (!s || s->role != SWMP_ROLE_HOST ||
        (s->state != SWMP_SESSION_LOBBY && s->state != SWMP_SESSION_PLAYING))
        return -1;
    for (uint8_t i = 1; i < SWMP_MAX_PEERS; i++) {
        if (s->peers[i].state != SWMP_PEER_EMPTY) continue;
        s->peers[i].state = SWMP_PEER_READY;
        s->peers[i].sequence_seen = 0;
        memset(s->peers[i].last_sequence, 0, sizeof s->peers[i].last_sequence);
        copy_name(s->peers[i].name, name);
        return i;
    }
    return -1;
}

bool swmp_session_remove_peer(SwmpSession *s, uint8_t peer_id) {
    if (!s || peer_id >= SWMP_MAX_PEERS || s->peers[peer_id].state == SWMP_PEER_EMPTY ||
        peer_id == s->local_peer)
        return false;
    if (peer_id == s->host_peer) {
        swmp_session_init(s);  // host migration is deliberately outside protocol v1
        return true;
    }
    SwmpPeer empty = {.id = peer_id};
    s->peers[peer_id] = empty;
    return true;
}

const SwmpPeer *swmp_session_peer(const SwmpSession *s, uint8_t peer_id) {
    if (!s || peer_id >= SWMP_MAX_PEERS || s->peers[peer_id].state == SWMP_PEER_EMPTY) return NULL;
    return &s->peers[peer_id];
}

size_t swmp_session_peer_count(const SwmpSession *s) {
    if (!s) return 0;
    size_t count = 0;
    for (uint8_t i = 0; i < SWMP_MAX_PEERS; i++) count += s->peers[i].state != SWMP_PEER_EMPTY;
    return count;
}

bool swmp_session_start(SwmpSession *s) {
    if (!s || s->role != SWMP_ROLE_HOST || s->state != SWMP_SESSION_LOBBY) return false;
    s->state = SWMP_SESSION_PLAYING;
    return true;
}

bool swmp_session_remote_start(SwmpSession *s) {
    if (!s || s->role != SWMP_ROLE_CLIENT || s->state != SWMP_SESSION_LOBBY) return false;
    s->state = SWMP_SESSION_PLAYING;
    return true;
}

bool swmp_session_next_sequence(SwmpSession *s, SwmpChannel channel, uint32_t *sequence) {
    if (!s || !sequence || s->state == SWMP_SESSION_OFFLINE || channel >= SWMP_CHANNEL_COUNT) return false;
    *sequence = s->next_sequence[channel]++;
    return true;
}

SwmpReceiveDecision swmp_session_accept(const SwmpSession *s, const SwmpPacketHeader *h) {
    if (!s || !h || s->state == SWMP_SESSION_OFFLINE || s->local_peer == SWMP_UNASSIGNED_PEER)
        return SWMP_RECEIVE_DROP_STATE;
    if (h->session_id != s->session_id) return SWMP_RECEIVE_DROP_SESSION;
    if (h->sender_peer >= SWMP_MAX_PEERS || h->sender_peer == s->local_peer ||
        s->peers[h->sender_peer].state != SWMP_PEER_READY || h->channel >= SWMP_CHANNEL_COUNT)
        return SWMP_RECEIVE_DROP_PEER;
    const SwmpPeer *peer = &s->peers[h->sender_peer];
    uint8_t channel_bit = (uint8_t)(1u << h->channel);
    if ((peer->sequence_seen & channel_bit) &&
        (h->sequence == peer->last_sequence[h->channel] ||
         (int32_t)(h->sequence - peer->last_sequence[h->channel]) <= 0))
        return SWMP_RECEIVE_DROP_STALE;
    return SWMP_RECEIVE_ACCEPT;
}

SwmpReceiveDecision swmp_session_receive(SwmpSession *s, const SwmpPacketHeader *h) {
    SwmpReceiveDecision decision = swmp_session_accept(s, h);
    if (decision != SWMP_RECEIVE_ACCEPT) return decision;
    SwmpPeer *peer = &s->peers[h->sender_peer];
    peer->last_sequence[h->channel] = h->sequence;
    peer->sequence_seen |= (uint8_t)(1u << h->channel);
    return SWMP_RECEIVE_ACCEPT;
}

