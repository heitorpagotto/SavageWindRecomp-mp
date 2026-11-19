#include "lobby.h"

#include <stdio.h>
#include <string.h>

void swmp_lobby_init(SwmpLobby *l, SwmpTransport transport) {
    *l = (SwmpLobby){.transport = transport};
    swmp_session_init(&l->session);
}

void swmp_lobby_leave(SwmpLobby *l) {
    SwmpTransport transport = l->transport;
    if (transport.stop) transport.stop(transport.context);
    swmp_lobby_init(l, transport);
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

static void disconnected(SwmpLobby *l, SwmpLobbyLink *link) {
    if (!link) return;
    uint8_t peer = link->peer;
    *link = (SwmpLobbyLink){0};
    if (l->session.role != SWMP_ROLE_HOST) {
        fail(l, SWMP_REJECT_NONE, "Host disconnected");
    } else if (swmp_session_remove_peer(&l->session, peer)) {
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

static void hello(SwmpLobby *l, SwmpLobbyLink *link, const SwmpControl *m) {
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

    SwmpControl welcome = {
        .welcome.local_peer = (uint8_t)peer,
        .welcome.playing = s->state == SWMP_SESSION_PLAYING,
        .welcome.features = l->features & m->hello.compatibility.features,
    };
    for (uint8_t i = 0; i < SWMP_MAX_PEERS; i++)
        if (swmp_session_peer(s, i)) strcpy(welcome.welcome.names[i], s->peers[i].name);
    if (!send_control(l, link, SWMP_MSG_WELCOME, &welcome)) {
        swmp_session_remove_peer(s, (uint8_t)peer);
        close_link(l, link);
        return;
    }
    link->peer = (uint8_t)peer;

    SwmpControl joined = {.peer.id = (uint8_t)peer};
    strcpy(joined.peer.name, m->hello.name);
    broadcast(l, SWMP_MSG_PEER_JOINED, &joined, (uint8_t)peer);
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
    link->peer = SWMP_HOST_PEER;
    l->joining = false;
}

static void packet(SwmpLobby *l, const SwmpTransportEvent *event, SwmpLobbyLink *link) {
    SwmpPacketHeader h;
    SwmpControl m;
    if (!link || link->closing || event->channel != SWMP_CHANNEL_CONTROL ||
        swmp_packet_decode(&h, event->data, event->size) != SWMP_PACKET_OK ||
        h.channel != SWMP_CHANNEL_CONTROL || h.flags != 0 ||
        !swmp_control_decode(&m, (SwmpMessageType)h.type, event->data + SWMP_PACKET_HEADER_SIZE, h.payload_size))
        return;
    if (l->joining) {
        if (h.sender_peer != SWMP_HOST_PEER || !h.session_id) return;
        if (h.type == SWMP_MSG_WELCOME) welcome(l, link, &h, &m);
        else if (h.type == SWMP_MSG_REJECT) fail(l, m.reject.code, m.reject.text);
        return;
    }
    bool host = l->session.role == SWMP_ROLE_HOST;
    if (host && link->peer == SWMP_UNASSIGNED_PEER) {
        if (h.type == SWMP_MSG_HELLO && h.sender_peer == SWMP_UNASSIGNED_PEER && !h.session_id && !h.sequence)
            hello(l, link, &m);
        return;
    }
    if (h.sender_peer != link->peer) return;
    if (host && h.type != SWMP_MSG_PING && h.type != SWMP_MSG_PONG) return;
    SwmpSession *s = &l->session;
    if (swmp_session_accept(s, &h) != SWMP_RECEIVE_ACCEPT) return;
    switch (h.type) {
    case SWMP_MSG_PEER_JOINED:
        if (!swmp_session_add_remote_peer(s, m.peer.id, m.peer.name)) return;
        break;
    case SWMP_MSG_PEER_LEFT:
        if (!swmp_session_remove_peer(s, m.peer.id)) return;
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
    if (l->joining && now - l->join_started_at >= SWMP_LOBBY_TIMEOUT_MS) {
        fail(l, SWMP_REJECT_NONE, "Lobby handshake timed out");
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
            disconnected(l, link);
            break;
        case SWMP_TRANSPORT_PACKET:
            packet(l, &event, link);
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
