#include "lobby.h"

#include <stdio.h>
#include <string.h>

void swmp_lobby_init(SwmpLobby *l, SwmpTransport transport) {
    *l = (SwmpLobby){.transport = transport};
    swmp_session_init(&l->session);
}

void swmp_lobby_leave(SwmpLobby *l) {
    SwmpTransport transport = l->transport;
    void (*peer_changed)(const char *, bool, uint64_t) = l->peer_changed;
    if (transport.stop) transport.stop(transport.context);
    swmp_lobby_init(l, transport);
    l->peer_changed = peer_changed;
}

static void fail(SwmpLobby *l, SwmpRejectCode code, const char *reason) {
    char error[SWMP_TRANSPORT_ERROR_SIZE] = {0};
    snprintf(error, sizeof error, "%s", reason);
    swmp_lobby_leave(l);
    l->rejection = code;
    memcpy(l->error, error, sizeof l->error);
}

static bool prepare(SwmpLobby *l, const SwmpCompatibility *c, const char *name) {
    if (l->joining || l->session.state != SWMP_SESSION_OFFLINE) return false;
    l->rejection = SWMP_REJECT_NONE;
    l->error[0] = 0;
    if (!c || c->protocol != SWMP_PROTOCOL_VERSION ||
        (c->required_features & ~c->features) || !swmp_player_name_valid(name)) {
        snprintf(l->error, sizeof l->error, "Invalid lobby configuration");
        return false;
    }
    const SwmpTransport *t = &l->transport;
    if (!t->start_host || !t->start_join || !t->poll || !t->send || !t->disconnect || !t->stop) {
        snprintf(l->error, sizeof l->error, "Lobby transport is unavailable");
        return false;
    }
    l->compatibility = *c;
    strcpy(l->name, name);
    return true;
}

bool swmp_lobby_host(SwmpLobby *l, const SwmpCompatibility *c, const char *name,
                    const char *url, uint64_t id) {
    if (!prepare(l, c, name)) return false;
    if (!id) {
        snprintf(l->error, sizeof l->error, "Invalid session ID");
        return false;
    }
    SwmpTransport *t = &l->transport;
    if (!t->start_host(t->context, url, l->error, sizeof l->error)) return false;
    l->features = c->features;
    return swmp_session_host(&l->session, id, l->name);
}

bool swmp_lobby_join(SwmpLobby *l, const SwmpCompatibility *c, const char *name,
                    const char *url, const char *invite, uint64_t now) {
    if (!prepare(l, c, name)) return false;
    SwmpTransport *t = &l->transport;
    if (!t->start_join(t->context, url, invite, l->error, sizeof l->error)) return false;
    l->joining = true;
    l->join_started_at = now;
    return true;
}

static bool send_control(SwmpLobby *l, const SwmpLobbyLink *link, SwmpMessageType type, const SwmpControl *msg) {
    uint8_t packet[SWMP_PACKET_HEADER_SIZE + SWMP_CONTROL_MAX_SIZE];
    size_t size;
    if (!swmp_control_encode(packet + SWMP_PACKET_HEADER_SIZE, SWMP_CONTROL_MAX_SIZE, &size, type, msg))
        return false;
    SwmpSession *s = &l->session;
    SwmpPacketHeader h = {
        .version = SWMP_PROTOCOL_VERSION,
        .type = (uint8_t)type,
        .payload_size = (uint32_t)size,
        .session_id = s->session_id,
        .sender_peer = s->local_peer,
        .channel = SWMP_CHANNEL_CONTROL,
        .tick = l->roster_epoch,
    };
    if (type != SWMP_MSG_HELLO && !swmp_session_next_sequence(s, SWMP_CHANNEL_CONTROL, &h.sequence))
        return false;
    size += SWMP_PACKET_HEADER_SIZE;
    return swmp_packet_encode(packet, size, &h) == SWMP_PACKET_OK &&
           l->transport.send(l->transport.context, link->connection, SWMP_CHANNEL_CONTROL, packet, size);
}

static SwmpLobbyLink *find_link(SwmpLobby *l, SwmpConnection connection) {
    if (!connection) return NULL;
    for (size_t i = 0; i < SWMP_LOBBY_LINKS; i++)
        if (l->links[i].connection == connection) return &l->links[i];
    return NULL;
}

static void close_link(SwmpLobby *l, SwmpLobbyLink *link) {
    link->closing = true;
    l->transport.disconnect(l->transport.context, link->connection);
}

static void broadcast(SwmpLobby *l, SwmpMessageType type, const SwmpControl *msg, uint8_t except) {
    for (size_t i = 0; i < SWMP_LOBBY_LINKS; i++) {
        SwmpLobbyLink *link = &l->links[i];
        if (!link->connection || link->closing || link->peer == SWMP_UNASSIGNED_PEER || link->peer == except)
            continue;
        if (!send_control(l, link, type, msg)) close_link(l, link);
    }
}

static void connected(SwmpLobby *l, const SwmpTransportEvent *event, uint64_t now) {
    if (!event->connection || find_link(l, event->connection)) return;
    // A client has a single authenticated route to the host.
    if (l->session.role != SWMP_ROLE_HOST && (!l->joining || l->links[0].connection)) {
        l->transport.disconnect(l->transport.context, event->connection);
        return;
    }
    for (size_t i = 0; i < SWMP_LOBBY_LINKS; i++) {
        SwmpLobbyLink *link = &l->links[i];
        if (link->connection) continue;
        *link = (SwmpLobbyLink){.connection = event->connection, .connected_at = now, .peer = SWMP_UNASSIGNED_PEER};
        memcpy(link->binding, event->binding, sizeof link->binding);
        if (l->joining) {
            l->waiting_welcome = true;
            l->join_started_at = now;
            SwmpControl msg = {.hello.compatibility = l->compatibility};
            memcpy(msg.hello.binding, link->binding, sizeof msg.hello.binding);
            strcpy(msg.hello.name, l->name);
            if (!send_control(l, link, SWMP_MSG_HELLO, &msg))
                fail(l, SWMP_REJECT_NONE, "Could not send lobby handshake");
        }
        return;
    }
    l->transport.disconnect(l->transport.context, event->connection);
}

static bool remove_peer(SwmpLobby *l, uint8_t id, uint64_t now) {
    const SwmpPeer *peer = swmp_session_peer(&l->session, id);
    if (!peer) return false;
    SwmpPeer departed = *peer;
    if (!swmp_session_remove_peer(&l->session, id)) return false;
    l->positions[id] = (SwmpPosition){0};
    l->equipment[id] = (SwmpEquipment){0};
    if (l->peer_changed) l->peer_changed(departed.name, false, now);
    return true;
}

static void disconnected(SwmpLobby *l, SwmpLobbyLink *link, uint64_t now) {
    if (!link) return;
    uint8_t peer = link->peer;
    *link = (SwmpLobbyLink){0};
    if (l->session.role != SWMP_ROLE_HOST) {
        const SwmpPeer *host = swmp_session_peer(&l->session, peer);
        if (host && l->peer_changed) l->peer_changed(host->name, false, now);
        fail(l, SWMP_REJECT_NONE, "Host disconnected");
    } else if (remove_peer(l, peer, now)) {
        ++l->roster_epoch;
        SwmpControl msg = {.peer.id = peer};
        broadcast(l, SWMP_MSG_PEER_LEFT, &msg, SWMP_UNASSIGNED_PEER);
    }
}

static SwmpRejectCode compatibility(const SwmpCompatibility *host, const SwmpCompatibility *client) {
    if (host->protocol != client->protocol) return SWMP_REJECT_PROTOCOL;
    if (memcmp(host->mod_version, client->mod_version, sizeof host->mod_version) ||
        memcmp(host->game_version, client->game_version, sizeof host->game_version) ||
        memcmp(host->savage_build, client->savage_build, sizeof host->savage_build) ||
        memcmp(host->game_build, client->game_build, sizeof host->game_build))
        return SWMP_REJECT_BUILD;
    if ((host->required_features & ~client->features) || (client->required_features & ~host->features))
        return SWMP_REJECT_FEATURES;
    return SWMP_REJECT_NONE;
}

static void reject(SwmpLobby *l, SwmpLobbyLink *link, SwmpRejectCode code, const char *text) {
    SwmpControl msg = {.reject.code = code};
    snprintf(msg.reject.text, sizeof msg.reject.text, "%s", text);
    // Let the response reach the client. Its own stop or the handshake deadline closes the link.
    if (!send_control(l, link, SWMP_MSG_REJECT, &msg)) close_link(l, link);
    link->closing = true;
}

static void hello(SwmpLobby *l, SwmpLobbyLink *link, const SwmpControl *m, uint64_t now) {
    static const char *const reasons[] = {
        [SWMP_REJECT_PROTOCOL] = "Incompatible multiplayer protocol",
        [SWMP_REJECT_BUILD] = "Incompatible game, Savage Wind, or mod build",
        [SWMP_REJECT_FEATURES] = "Required multiplayer features are unavailable",
    };
    if (memcmp(link->binding, m->hello.binding, sizeof link->binding)) {
        reject(l, link, SWMP_REJECT_INVALID, "Invalid lobby connection binding");
        return;
    }
    SwmpRejectCode code = compatibility(&l->compatibility, &m->hello.compatibility);
    if (code != SWMP_REJECT_NONE) {
        reject(l, link, code, reasons[code]);
        return;
    }
    SwmpSession *s = &l->session;
    int peer = swmp_session_add_peer(s, m->hello.name);
    if (peer < 0) {
        reject(l, link, SWMP_REJECT_FULL, "Lobby is full");
        return;
    }

    ++l->roster_epoch;
    SwmpControl welcome = {
        .welcome.local_peer = (uint8_t)peer,
        .welcome.playing = s->state == SWMP_SESSION_PLAYING,
        .welcome.features = l->features & m->hello.compatibility.features,
    };
    for (uint8_t i = 0; i < SWMP_MAX_PEERS; i++)
        if (swmp_session_peer(s, i)) strcpy(welcome.welcome.names[i], s->peers[i].name);
    if (!send_control(l, link, SWMP_MSG_WELCOME, &welcome)) {
        swmp_session_remove_peer(s, (uint8_t)peer);
        --l->roster_epoch;
        close_link(l, link);
        return;
    }
    link->peer = (uint8_t)peer;

    // Use the membership channel so equipment cannot arrive ahead of WELCOME or PEER_JOINED.
    for (uint8_t i = 0; i < SWMP_MAX_PEERS; i++) {
        if (!l->equipment[i].revision || !swmp_session_peer(s, i)) continue;
        SwmpControl equipment = {.equipment = {.peer = i, .state = l->equipment[i]}};
        if (!send_control(l, link, SWMP_MSG_EQUIPMENT, &equipment)) {
            close_link(l, link);
            break;
        }
    }
    SwmpControl joined = {.peer.id = (uint8_t)peer};
    strcpy(joined.peer.name, m->hello.name);
    broadcast(l, SWMP_MSG_PEER_JOINED, &joined, (uint8_t)peer);
    if (l->peer_changed) l->peer_changed(s->peers[peer].name, true, now);
}

static void welcome(SwmpLobby *l, SwmpLobbyLink *link, const SwmpPacketHeader *h, const SwmpControl *m) {
    uint8_t local = m->welcome.local_peer;
    uint32_t features = m->welcome.features;
    if (strcmp(l->name, m->welcome.names[local]) || (features & ~l->compatibility.features) ||
        (l->compatibility.required_features & ~features)) {
        fail(l, SWMP_REJECT_INVALID, "Invalid lobby welcome");
        return;
    }
    SwmpSession *s = &l->session;
    if (!swmp_session_join(s, h->session_id, l->name) ||
        !swmp_session_client_welcome(s, local, m->welcome.names[SWMP_HOST_PEER])) {
        fail(l, SWMP_REJECT_INVALID, "Invalid lobby session");
        return;
    }
    for (uint8_t i = 1; i < SWMP_MAX_PEERS; i++)
        if (i != local && m->welcome.names[i][0]) swmp_session_add_remote_peer(s, i, m->welcome.names[i]);
    if (m->welcome.playing) swmp_session_remote_start(s);
    swmp_session_receive(s, h);
    l->features = features;
    l->roster_epoch = h->tick;
    link->peer = SWMP_HOST_PEER;
    l->joining = false;
}

bool swmp_position_fresh(const SwmpPosition *position, uint64_t now) {
    return position->snapshot.area_id && now - position->received_at < SWMP_POSITION_TIMEOUT_MS;
}

void swmp_lobby_set_equipment(SwmpLobby *l, SwmpGearSet gear) {
    SwmpSession *s = &l->session;
    if (!s->session_id || l->joining || s->local_peer >= SWMP_MAX_PEERS || !swmp_gear_set_valid(gear))
        return;
    SwmpEquipment *local = &l->equipment[s->local_peer];
    if (!memcmp(local->gear.id, gear.id, sizeof gear.id) &&
        !memcmp(local->gear.armor, gear.armor, sizeof gear.armor) &&
        !memcmp(local->gear.armor_dye, gear.armor_dye, sizeof gear.armor_dye)) return;
    local->gear = gear;
    if (!++local->revision) ++local->revision;
    SwmpControl msg = {.equipment = {.peer = s->local_peer, .state = *local}};
    broadcast(l, SWMP_MSG_EQUIPMENT, &msg, SWMP_UNASSIGNED_PEER);
}

void swmp_lobby_send_position(SwmpLobby *l, const SwmpPlayerSnapshot *local, uint64_t now) {
    swmp_lobby_send_state(l, local, NULL, now);
}

void swmp_lobby_send_state(SwmpLobby *l, const SwmpPlayerSnapshot *local,
                           const SwmpEnvironment *environment, uint64_t now) {
    SwmpSession *s = &l->session;
    if (!s->session_id || l->joining || now < l->next_position_at ||
        swmp_session_peer_count(s) < 2) return;
    l->next_position_at = now + SWMP_POSITION_INTERVAL_MS;
    l->positions[s->local_peer] = (SwmpPosition){.snapshot = *local, .received_at = now};
    SwmpPlayerSnapshot *sample = &l->positions[s->local_peer].snapshot;
    sample->peer_id = s->local_peer;
    const SwmpEquipment *equipment = &l->equipment[s->local_peer];
    sample->equipment_revision = equipment->revision;
    for (unsigned slot = 0; slot < SWMP_GEAR_SLOTS; slot++)
        if (!equipment->gear.id[slot]) sample->flags &= ~swmp_gear_pose_flag(slot);
    if (!(sample->flags & SWMP_PLAYER_BOW_POSE)) {
        sample->nocked_arrow = SWMP_ARROW_NONE;
        sample->bow_animation = SWMP_BOW_ANIM_NONE;
    }
    if (!(sample->flags & SWMP_PLAYER_SHIELD_POSE)) sample->shield_animation = SWMP_SHIELD_ANIM_NONE;

    uint8_t data[SWMP_PACKET_HEADER_SIZE + SWMP_ENVIRONMENT_SIZE + SWMP_MAX_PEERS * SWMP_PLAYER_SNAPSHOT_SIZE];
    size_t size = 0;
    bool host = s->role == SWMP_ROLE_HOST;
    if (host && local->area_id && swmp_environment_encode(data + SWMP_PACKET_HEADER_SIZE, environment))
        size = SWMP_ENVIRONMENT_SIZE;
    // Each host datagram replaces the entire state for this link. A dropped datagram or a late join
    // recovers from the next one without letting one player's sample overwrite another's in the transport.
    for (uint8_t i = 0; i < SWMP_MAX_PEERS; i++) {
        const SwmpPosition *p = &l->positions[i];
        if ((i != s->local_peer && !host) || !swmp_position_fresh(p, now)) continue;
        if (swmp_player_snapshot_encode(data + SWMP_PACKET_HEADER_SIZE + size, &p->snapshot))
            size += SWMP_PLAYER_SNAPSHOT_SIZE;
    }
    SwmpPacketHeader h = {
        .version = SWMP_PROTOCOL_VERSION, .type = SWMP_MSG_PLAYER_SNAPSHOT, .payload_size = (uint32_t)size,
        .session_id = s->session_id, .sender_peer = s->local_peer, .channel = SWMP_CHANNEL_STATE,
        .tick = host ? l->roster_epoch : 0,
    };
    swmp_session_next_sequence(s, SWMP_CHANNEL_STATE, &h.sequence);
    size += SWMP_PACKET_HEADER_SIZE;
    if (swmp_packet_encode(data, size, &h) != SWMP_PACKET_OK) return;
    for (size_t i = 0; i < SWMP_LOBBY_LINKS; i++) {
        const SwmpLobbyLink *link = &l->links[i];
        if (link->connection && !link->closing && link->peer != SWMP_UNASSIGNED_PEER)
            l->transport.send(l->transport.context, link->connection, SWMP_CHANNEL_STATE, data, size);
    }
}

static void position_packet(SwmpLobby *l, const SwmpPacketHeader *h, const uint8_t *data, uint64_t now) {
    bool host = l->session.role == SWMP_ROLE_HOST;
    size_t size = h->payload_size;
    SwmpHostEnvironment environment = {0};
    bool has_environment = !host && size % SWMP_PLAYER_SNAPSHOT_SIZE == SWMP_ENVIRONMENT_SIZE;
    if (has_environment) {
        if (!swmp_environment_decode(&environment.state, data)) return;
        data += SWMP_ENVIRONMENT_SIZE;
        size -= SWMP_ENVIRONMENT_SIZE;
    }
    if (size % SWMP_PLAYER_SNAPSHOT_SIZE || size > SWMP_MAX_PEERS * SWMP_PLAYER_SNAPSHOT_SIZE ||
        (host ? size > SWMP_PLAYER_SNAPSHOT_SIZE || h->tick : h->tick != l->roster_epoch) ||
        swmp_session_accept(&l->session, h) != SWMP_RECEIVE_ACCEPT) return;
    SwmpPosition positions[SWMP_MAX_PEERS] = {0};
    while (size) {
        SwmpPlayerSnapshot p;
        if (!swmp_player_snapshot_decode(&p, data) || !p.area_id ||
            (host && p.peer_id != h->sender_peer) || !swmp_session_peer(&l->session, p.peer_id) ||
            positions[p.peer_id].snapshot.area_id) return;
        positions[p.peer_id] = (SwmpPosition){.snapshot = p, .received_at = now};
        data += SWMP_PLAYER_SNAPSHOT_SIZE;
        size -= SWMP_PLAYER_SNAPSHOT_SIZE;
    }
    if (has_environment) {
        environment.area_id = positions[SWMP_HOST_PEER].snapshot.area_id;
        if (!environment.area_id) return;
        environment.received_at = now;
    }
    swmp_session_receive(&l->session, h);
    if (host) {
        l->positions[h->sender_peer] = positions[h->sender_peer];
    } else {
        l->environment = environment;
        for (uint8_t i = 0; i < SWMP_MAX_PEERS; i++)
            if (i != l->session.local_peer) l->positions[i] = positions[i];
    }
}

static void packet(SwmpLobby *l, const SwmpTransportEvent *event, SwmpLobbyLink *link, uint64_t now) {
    SwmpPacketHeader h;
    SwmpControl m;
    if (!link || link->closing || swmp_packet_decode(&h, event->data, event->size) != SWMP_PACKET_OK ||
        h.channel != event->channel || h.flags != 0) return;
    const uint8_t *data = event->data + SWMP_PACKET_HEADER_SIZE;
    if (h.type == SWMP_MSG_PLAYER_SNAPSHOT) {
        if (!l->joining && h.sender_peer == link->peer) position_packet(l, &h, data, now);
        return;
    }
    if (h.channel != SWMP_CHANNEL_CONTROL ||
        !swmp_control_decode(&m, (SwmpMessageType)h.type, data, h.payload_size)) return;
    if (l->joining) {
        if (h.sender_peer != SWMP_HOST_PEER || !h.session_id) return;
        if (h.type == SWMP_MSG_WELCOME) welcome(l, link, &h, &m);
        else if (h.type == SWMP_MSG_REJECT) fail(l, m.reject.code, m.reject.text);
        return;
    }
    bool host = l->session.role == SWMP_ROLE_HOST;
    if (host && link->peer == SWMP_UNASSIGNED_PEER) {
        if (h.type == SWMP_MSG_HELLO && h.sender_peer == SWMP_UNASSIGNED_PEER && !h.session_id && !h.sequence)
            hello(l, link, &m, now);
        return;
    }
    if (h.sender_peer != link->peer) return;
    if (host && h.type != SWMP_MSG_PING && h.type != SWMP_MSG_PONG && h.type != SWMP_MSG_EQUIPMENT) return;
    SwmpSession *s = &l->session;
    if (swmp_session_accept(s, &h) != SWMP_RECEIVE_ACCEPT) return;
    switch (h.type) {
    case SWMP_MSG_EQUIPMENT: {
        uint8_t peer = m.equipment.peer;
        if ((host && peer != h.sender_peer) || peer == s->local_peer || !swmp_session_peer(s, peer)) return;
        SwmpEquipment *state = &l->equipment[peer];
        uint32_t revision = m.equipment.state.revision;
        if (state->revision && (int32_t)(revision - state->revision) <= 0) return;
        *state = m.equipment.state;
        if (host) broadcast(l, SWMP_MSG_EQUIPMENT, &m, peer);
        break;
    }
    case SWMP_MSG_PEER_JOINED:
        if (!swmp_session_add_remote_peer(s, m.peer.id, m.peer.name)) return;
        l->roster_epoch = h.tick;
        if (l->peer_changed) l->peer_changed(s->peers[m.peer.id].name, true, now);
        break;
    case SWMP_MSG_PEER_LEFT:
        if (!remove_peer(l, m.peer.id, now)) return;
        l->roster_epoch = h.tick;
        break;
    case SWMP_MSG_START:
        if (!swmp_session_remote_start(s)) return;
        break;
    case SWMP_MSG_PING:
        if (!send_control(l, link, SWMP_MSG_PONG, &m)) close_link(l, link);
        break;
    case SWMP_MSG_PONG:
        break;
    default:
        return;
    }
    swmp_session_receive(s, &h);
}

void swmp_lobby_poll(SwmpLobby *l, uint64_t now) {
    uint64_t connect_timeout = l->transport.connect_timeout_ms;
    uint64_t join_timeout = l->waiting_welcome || !connect_timeout ? SWMP_LOBBY_TIMEOUT_MS : connect_timeout;
    if (l->joining && now - l->join_started_at >= join_timeout) {
        fail(l, SWMP_REJECT_NONE, l->waiting_welcome || !connect_timeout ?
             "Lobby handshake timed out" : "Lobby connection timed out");
        return;
    }
    if (!l->joining && l->session.state == SWMP_SESSION_OFFLINE) return;
    for (size_t i = 0; i < SWMP_LOBBY_LINKS; i++) {
        SwmpLobbyLink *link = &l->links[i];
        if (link->connection && link->peer == SWMP_UNASSIGNED_PEER &&
            now - link->connected_at >= SWMP_LOBBY_TIMEOUT_MS)
            close_link(l, link);
    }
    SwmpTransportEvent event;
    // A busy backend must not monopolize the caller's update.
    for (unsigned i = 0; i < 64 && l->transport.poll(l->transport.context, &event); i++) {
        SwmpLobbyLink *link = find_link(l, event.connection);
        switch (event.type) {
        case SWMP_TRANSPORT_INVITE_READY:
            if (event.text && l->session.role == SWMP_ROLE_HOST)
                snprintf(l->invite, sizeof l->invite, "%s", event.text);
            break;
        case SWMP_TRANSPORT_PEER_CONNECTED:
            connected(l, &event, now);
            break;
        case SWMP_TRANSPORT_PEER_DISCONNECTED:
            disconnected(l, link, now);
            break;
        case SWMP_TRANSPORT_PACKET:
            packet(l, &event, link, now);
            break;
        case SWMP_TRANSPORT_ERROR:
            fail(l, SWMP_REJECT_NONE, event.text ? event.text : "Lobby transport failed");
            break;
        }
        if (!l->joining && l->session.state == SWMP_SESSION_OFFLINE) return;
    }
}

bool swmp_lobby_start(SwmpLobby *l) {
    if (!swmp_session_start(&l->session)) return false;
    broadcast(l, SWMP_MSG_START, NULL, SWMP_UNASSIGNED_PEER);
    return true;
}
