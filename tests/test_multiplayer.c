#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "lobby.h"
#include "mock_transport.h"

static int failures;

#define CHECK(x) do {                                                                                                  \
    if (!(x)) {                                                                                                        \
        fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #x);                                        \
        failures++;                                                                                                    \
    }                                                                                                                  \
} while (0)

static void packet_test(void) {
    uint8_t packet[SWMP_PACKET_HEADER_SIZE + 3] = {};
    SwmpPacketHeader in = {
        .version = SWMP_PROTOCOL_VERSION,
        .type = SWMP_MSG_PLAYER_SNAPSHOT,
        .flags = SWMP_FLAG_CRITICAL,
        .payload_size = 3,
        .session_id = UINT64_C(0x0123456789abcdef),
        .sender_peer = 3,
        .channel = SWMP_CHANNEL_STATE,
        .sequence = UINT32_C(0x10203040),
        .tick = UINT32_C(0xa0b0c0d0),
    };
    CHECK(swmp_packet_encode(packet, sizeof packet, &in) == SWMP_PACKET_OK);
    const uint8_t expected[] = {
        0x53, 0x57, 0x4d, 0x50, 0, 1, 16, 4, 0, 0, 0, 3,
        0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 3, 1, 0, 0,
        0x10, 0x20, 0x30, 0x40, 0xa0, 0xb0, 0xc0, 0xd0,
    };
    CHECK(!memcmp(packet, expected, sizeof expected));
    SwmpPacketHeader out = {};
    CHECK(swmp_packet_decode(&out, packet, sizeof packet) == SWMP_PACKET_OK);
    CHECK(out.version == in.version && out.type == in.type && out.flags == in.flags);
    CHECK(out.payload_size == in.payload_size && out.session_id == in.session_id);
    CHECK(out.sender_peer == in.sender_peer && out.channel == in.channel);
    CHECK(out.sequence == in.sequence && out.tick == in.tick);

    CHECK(swmp_packet_decode(&out, packet, SWMP_PACKET_HEADER_SIZE) == SWMP_PACKET_SIZE_MISMATCH);
    packet[0] = 0;
    CHECK(swmp_packet_decode(&out, packet, sizeof packet) == SWMP_PACKET_BAD_MAGIC);
    packet[0] = 0x53;
    packet[5] = 2;
    CHECK(swmp_packet_decode(&out, packet, sizeof packet) == SWMP_PACKET_BAD_VERSION);
    packet[5] = 1;
    packet[21] = SWMP_CHANNEL_COUNT;
    CHECK(swmp_packet_decode(&out, packet, sizeof packet) == SWMP_PACKET_BAD_CHANNEL);
    packet[21] = SWMP_CHANNEL_CONTROL;
    CHECK(swmp_packet_decode(&out, packet, sizeof packet) == SWMP_PACKET_BAD_CHANNEL);
    packet[21] = SWMP_CHANNEL_STATE;
    packet[7] = SWMP_FLAG_REQUEST | SWMP_FLAG_RESPONSE;
    CHECK(swmp_packet_decode(&out, packet, sizeof packet) == SWMP_PACKET_BAD_FLAGS);
    packet[7] = SWMP_FLAG_CRITICAL;
    packet[22] = 1;
    CHECK(swmp_packet_decode(&out, packet, sizeof packet) == SWMP_PACKET_BAD_RESERVED);

    SwmpPacketHeader hello = in;
    hello.type = SWMP_MSG_HELLO;
    hello.sender_peer = SWMP_UNASSIGNED_PEER;
    hello.channel = SWMP_CHANNEL_CONTROL;
    CHECK(swmp_packet_encode(packet, sizeof packet, &hello) == SWMP_PACKET_OK);
    hello.type = SWMP_MSG_PING;
    CHECK(swmp_packet_encode(packet, sizeof packet, &hello) == SWMP_PACKET_BAD_PEER);
}

static void snapshot_test(void) {
    SwmpPlayerSnapshot in = {
        .peer_id = 7,
        .flags = SWMP_PLAYER_GROUNDED | SWMP_PLAYER_GUARDING,
        .action = 0x1234,
        .area_id = 0x89abcdef,
        .phase_id = 42,
        .position_mm = {INT32_MIN, -123456, INT32_MAX},
        .rotation = {INT16_MIN, -12, INT16_MAX},
        .velocity_cm_s = {-3000, 0, 3000},
        .health_q4 = 120,
        .stamina_q4 = 400,
        .equipment_revision = 0xfedcba98,
    };
    uint8_t data[SWMP_PLAYER_SNAPSHOT_SIZE];
    CHECK(swmp_player_snapshot_encode(data, &in));
    SwmpPlayerSnapshot out = {};
    CHECK(swmp_player_snapshot_decode(&out, data));
    CHECK(out.peer_id == in.peer_id && out.flags == in.flags && out.action == in.action);
    CHECK(out.area_id == in.area_id && out.phase_id == in.phase_id);
    CHECK(!memcmp(out.position_mm, in.position_mm, sizeof in.position_mm));
    CHECK(!memcmp(out.rotation, in.rotation, sizeof in.rotation));
    CHECK(!memcmp(out.velocity_cm_s, in.velocity_cm_s, sizeof in.velocity_cm_s));
    CHECK(out.health_q4 == in.health_q4 && out.stamina_q4 == in.stamina_q4);
    CHECK(out.equipment_revision == in.equipment_revision);
    data[1] = 0x80;
    CHECK(!swmp_player_snapshot_decode(&out, data));
}

static SwmpPacketHeader peer_packet(const SwmpSession *s, uint8_t peer, uint32_t sequence) {
    SwmpPacketHeader h = {
        .version = SWMP_PROTOCOL_VERSION,
        .type = SWMP_MSG_PLAYER_SNAPSHOT,
        .session_id = s->session_id,
        .sender_peer = peer,
        .channel = SWMP_CHANNEL_STATE,
        .sequence = sequence,
    };
    return h;
}

static void host_session_test(void) {
    SwmpSession s;
    swmp_session_init(&s);
    CHECK(s.state == SWMP_SESSION_OFFLINE && swmp_session_peer_count(&s) == 0);
    CHECK(!swmp_session_host(&s, 0, "Host"));
    CHECK(swmp_session_host(&s, 0x1234, "Host"));
    CHECK(s.role == SWMP_ROLE_HOST && s.state == SWMP_SESSION_LOBBY);
    CHECK(s.local_peer == SWMP_HOST_PEER && swmp_session_peer_count(&s) == 1);
    for (int i = 1; i < (int)SWMP_MAX_PEERS; i++) CHECK(swmp_session_add_peer(&s, "Link") == i);
    CHECK(swmp_session_peer_count(&s) == SWMP_MAX_PEERS);
    CHECK(swmp_session_add_peer(&s, "Too many") == -1);
    CHECK(swmp_session_start(&s) && s.state == SWMP_SESSION_PLAYING);
    CHECK(!swmp_session_start(&s));

    SwmpPacketHeader h = peer_packet(&s, 3, 10);
    CHECK(swmp_session_accept(&s, &h) == SWMP_RECEIVE_ACCEPT);  // inspection does not consume it
    CHECK(swmp_session_receive(&s, &h) == SWMP_RECEIVE_ACCEPT);
    CHECK(swmp_session_receive(&s, &h) == SWMP_RECEIVE_DROP_STALE);
    h.sequence = 9;
    CHECK(swmp_session_receive(&s, &h) == SWMP_RECEIVE_DROP_STALE);
    h.sequence = 11;
    CHECK(swmp_session_receive(&s, &h) == SWMP_RECEIVE_ACCEPT);
    h.session_id++;
    CHECK(swmp_session_receive(&s, &h) == SWMP_RECEIVE_DROP_SESSION);
    h.session_id = s.session_id;
    h.sender_peer = s.local_peer;
    CHECK(swmp_session_receive(&s, &h) == SWMP_RECEIVE_DROP_PEER);
    CHECK(swmp_session_remove_peer(&s, 3));
    h.sender_peer = 3;
    CHECK(swmp_session_receive(&s, &h) == SWMP_RECEIVE_DROP_PEER);
    CHECK(swmp_session_add_peer(&s, "Late Link") == 3);

    uint32_t sequence = 0;
    CHECK(swmp_session_next_sequence(&s, SWMP_CHANNEL_EVENT, &sequence) && sequence == 1);
    CHECK(swmp_session_next_sequence(&s, SWMP_CHANNEL_EVENT, &sequence) && sequence == 2);
}

static void client_session_test(void) {
    SwmpSession s;
    swmp_session_init(&s);
    CHECK(swmp_session_join(&s, 0x8877, "Client"));
    CHECK(s.role == SWMP_ROLE_CLIENT && s.state == SWMP_SESSION_CONNECTING);
    CHECK(!swmp_session_client_welcome(&s, 0, "Host"));
    CHECK(swmp_session_client_welcome(&s, 2, "Host"));
    CHECK(s.local_peer == 2 && swmp_session_peer_count(&s) == 2);
    CHECK(swmp_session_add_remote_peer(&s, 5, "Remote Link"));
    CHECK(!swmp_session_add_remote_peer(&s, 5, "Duplicate"));
    CHECK(swmp_session_peer_count(&s) == 3);
    CHECK(swmp_session_remote_start(&s) && s.state == SWMP_SESSION_PLAYING);
    CHECK(swmp_session_remove_peer(&s, SWMP_HOST_PEER));
    CHECK(s.state == SWMP_SESSION_OFFLINE && s.role == SWMP_ROLE_NONE);
}

static const SwmpCompatibility local_build = {
    .protocol = SWMP_PROTOCOL_VERSION,
    .mod_version = {0, 1, 0},
    .game_version = {1, 9, 0},
    .savage_build = {0x71},
    .game_build = {0x92},
    .features = 7,
    .required_features = 1,
};

static void control_test(void) {
    uint8_t data[SWMP_CONTROL_MAX_SIZE + 1];
    size_t size = 0;
    SwmpControl in = {.hello.compatibility = local_build, .hello.name = "L\xc3\xadnk"};
    SwmpControl out = {0};
    in.hello.binding[15] = 0xa5;
    CHECK(swmp_control_encode(data, sizeof data, &size, SWMP_MSG_HELLO, &in));
    CHECK(size == 96 && data[1] == 1 && data[14] == 0x71 && data[34] == 0x92 && data[89] == 0xa5);
    for (size_t n = 0; n < size; n++) CHECK(!swmp_control_decode(&out, SWMP_MSG_HELLO, data, n));
    CHECK(swmp_control_decode(&out, SWMP_MSG_HELLO, data, size));
    CHECK(!strcmp(in.hello.name, out.hello.name) && out.hello.compatibility.features == 7);
    CHECK(!swmp_control_decode(&out, SWMP_MSG_HELLO, data, size + 1));
    data[90] = 32;
    CHECK(!swmp_control_decode(&out, SWMP_MSG_HELLO, data, size));
    data[90] = 5;
    data[91] = 0;
    CHECK(!swmp_control_decode(&out, SWMP_MSG_HELLO, data, size));
    CHECK(!swmp_control_encode(data, size - 1, &size, SWMP_MSG_HELLO, &in));
    CHECK(swmp_player_name_valid("\xf0\x9f\x8e\xae"));
    CHECK(!swmp_player_name_valid("") && !swmp_player_name_valid("line\nbreak"));
    CHECK(!swmp_player_name_valid("\xc2\x9f") && !swmp_player_name_valid("\xe0\x80\xaf"));
    CHECK(!swmp_player_name_valid("\xed\xa0\x80") && !swmp_player_name_valid("\xf4\x90\x80\x80"));
    CHECK(!swmp_player_name_valid("\xe2\x82") && !swmp_player_name_valid("\x80"));

    in = (SwmpControl){.welcome.local_peer = 7, .welcome.playing = true, .welcome.features = 5};
    for (size_t i = 0; i < SWMP_MAX_PEERS; i++)
        memset(in.welcome.names[i], 'A' + (int)i, SWMP_PLAYER_NAME_SIZE - 1);
    CHECK(swmp_control_encode(data, sizeof data, &size, SWMP_MSG_WELCOME, &in));
    CHECK(size == SWMP_CONTROL_MAX_SIZE);
    CHECK(swmp_control_decode(&out, SWMP_MSG_WELCOME, data, size));
    CHECK(out.welcome.local_peer == 7 && out.welcome.playing && out.welcome.features == 5);
    CHECK(!memcmp(out.welcome.names, in.welcome.names, sizeof in.welcome.names));
    data[6] = 1;  // duplicate ID and no host
    CHECK(!swmp_control_decode(&out, SWMP_MSG_WELCOME, data, size));
    data[6] = 0;
    CHECK(!swmp_control_decode(&out, SWMP_MSG_WELCOME, data, size - 33));  // missing assigned peer

    in = (SwmpControl){.reject.code = SWMP_REJECT_FULL, .reject.text = "Lobby is full"};
    CHECK(swmp_control_encode(data, sizeof data, &size, SWMP_MSG_REJECT, &in));
    CHECK(swmp_control_decode(&out, SWMP_MSG_REJECT, data, size) && out.reject.code == SWMP_REJECT_FULL);
    data[1] = SWMP_REJECT_TEXT_SIZE;
    CHECK(!swmp_control_decode(&out, SWMP_MSG_REJECT, data, size));
    CHECK(swmp_control_encode(NULL, 0, &size, SWMP_MSG_START, NULL) && size == 0);
    CHECK(swmp_control_decode(&out, SWMP_MSG_START, NULL, 0));
    CHECK(!swmp_control_decode(&out, SWMP_MSG_START, data, 1));
    in = (SwmpControl){.peer.id = 4, .peer.name = "Remote"};
    CHECK(swmp_control_encode(data, sizeof data, &size, SWMP_MSG_PEER_JOINED, &in));
    CHECK(swmp_control_decode(&out, SWMP_MSG_PEER_JOINED, data, size));
    CHECK(out.peer.id == 4 && !strcmp(out.peer.name, "Remote"));
    CHECK(!swmp_control_decode(&out, SWMP_MSG_PEER_LEFT, data, size));
    CHECK(swmp_control_encode(data, sizeof data, &size, SWMP_MSG_PEER_LEFT, &in));
    CHECK(size == 1 && swmp_control_decode(&out, SWMP_MSG_PEER_LEFT, data, size) && out.peer.id == 4);
    data[0] = SWMP_HOST_PEER;
    CHECK(!swmp_control_decode(&out, SWMP_MSG_PEER_LEFT, data, size));
}

static bool raw_control(SwmpTransport *t, SwmpConnection connection, SwmpMessageType type, uint8_t sender,
                        uint64_t session, uint32_t sequence, const SwmpControl *msg) {
    uint8_t packet[SWMP_PACKET_HEADER_SIZE + SWMP_CONTROL_MAX_SIZE];
    size_t size;
    if (!swmp_control_encode(packet + SWMP_PACKET_HEADER_SIZE, SWMP_CONTROL_MAX_SIZE, &size, type, msg))
        return false;
    SwmpPacketHeader h = {
        .version = SWMP_PROTOCOL_VERSION, .type = (uint8_t)type, .payload_size = (uint32_t)size,
        .session_id = session, .sender_peer = sender, .channel = SWMP_CHANNEL_CONTROL, .sequence = sequence,
    };
    size += SWMP_PACKET_HEADER_SIZE;
    return swmp_packet_encode(packet, size, &h) == SWMP_PACKET_OK &&
           t->send(t->context, connection, SWMP_CHANNEL_CONTROL, packet, size);
}

static void lobby_test(void) {
    SwmpMock *mock = swmp_mock_create();
    CHECK(mock);
    if (!mock) return;
    SwmpLobby lobbies[SWMP_MAX_PEERS + 1];
    for (unsigned i = 0; i <= SWMP_MAX_PEERS; i++)
        swmp_lobby_init(&lobbies[i], swmp_mock_transport(mock, i));
    SwmpLobby *host = &lobbies[0], *extra = &lobbies[SWMP_MAX_PEERS];
    CHECK(swmp_lobby_host(host, &local_build, "Host", NULL, 0x9876));
    swmp_lobby_poll(host, 0);
    CHECK(host->invite[0]);
    SwmpCompatibility other = local_build;
    other.game_build[0]++;
    CHECK(swmp_lobby_join(extra, &other, "Extra", NULL, host->invite, 0));
    swmp_lobby_poll(extra, 0);
    swmp_lobby_poll(host, 0);
    swmp_lobby_poll(extra, 0);
    CHECK(!extra->joining && extra->rejection == SWMP_REJECT_BUILD);
    CHECK(swmp_session_peer_count(&host->session) == 1);
    CHECK(!swmp_session_remote_roster(&host->session).generation[1]);
    other = local_build;
    other.required_features = other.features = 8;
    CHECK(swmp_lobby_join(extra, &other, "Extra", NULL, host->invite, 0));
    swmp_lobby_poll(extra, 0);
    swmp_lobby_poll(host, 0);
    swmp_lobby_poll(extra, 0);
    CHECK(!extra->joining && extra->rejection == SWMP_REJECT_FEATURES);
    CHECK(!swmp_lobby_join(extra, &local_build, "Extra", NULL, "wrong invite", 0));
    for (unsigned i = 1; i <= SWMP_MAX_PEERS; i++) {
        char name[SWMP_PLAYER_NAME_SIZE];
        snprintf(name, sizeof name, "Link %u", i);
        other = local_build;
        other.features = 5;
        CHECK(swmp_lobby_join(&lobbies[i], &other, name, NULL, host->invite, 1));
        swmp_lobby_poll(&lobbies[i], 1);
    }
    swmp_lobby_poll(host, 1);
    for (unsigned i = 1; i < SWMP_MAX_PEERS; i++) {
        swmp_lobby_poll(&lobbies[i], 1);
        CHECK(lobbies[i].session.local_peer == i && lobbies[i].features == 5);
        CHECK(swmp_session_peer_count(&lobbies[i].session) == SWMP_MAX_PEERS);
        CHECK(!strcmp(lobbies[i].session.peers[7].name, "Link 7"));
    }
    swmp_lobby_poll(extra, 1);
    CHECK(extra->rejection == SWMP_REJECT_FULL);
    CHECK(swmp_lobby_start(host));

    SwmpRemoteRoster host_before = swmp_session_remote_roster(&host->session);
    SwmpRemoteRoster client_before = swmp_session_remote_roster(&lobbies[2].session);
    CHECK(!host_before.generation[0] && host_before.generation[3]);
    CHECK(client_before.generation[0] && !client_before.generation[2] && client_before.generation[3]);
    swmp_lobby_leave(&lobbies[3]);
    swmp_lobby_poll(host, 2);
    CHECK(!swmp_session_remote_roster(&host->session).generation[3]);

    // Reuse the same slot and display name before the observer polls either roster event.
    CHECK(swmp_lobby_join(extra, &local_build, "Link 3", NULL, host->invite, 2));
    CHECK(!swmp_session_remote_roster(&extra->session).generation[0]);
    swmp_lobby_poll(extra, 2);
    swmp_lobby_poll(host, 2);
    swmp_lobby_poll(extra, 2);
    CHECK(extra->session.state == SWMP_SESSION_PLAYING && extra->session.local_peer == 3);
    CHECK(swmp_session_peer_count(&extra->session) == SWMP_MAX_PEERS);
    SwmpRemoteRoster late = swmp_session_remote_roster(&extra->session);
    for (uint8_t i = 0; i < SWMP_MAX_PEERS; i++) CHECK((late.generation[i] != 0) == (i != 3));
    swmp_lobby_poll(&lobbies[2], 2);
    CHECK(lobbies[2].session.state == SWMP_SESSION_PLAYING);
    CHECK(!strcmp(lobbies[2].session.peers[3].name, "Link 3"));
    SwmpRemoteRoster host_after = swmp_session_remote_roster(&host->session);
    SwmpRemoteRoster client_after = swmp_session_remote_roster(&lobbies[2].session);
    CHECK(host_after.generation[3] > host_before.generation[3]);
    CHECK(client_after.generation[3] > client_before.generation[3]);
    CHECK(host_after.generation[1] == host_before.generation[1]);
    CHECK(client_after.generation[0] == client_before.generation[0]);

    swmp_lobby_leave(host);
    for (unsigned i = 1; i <= SWMP_MAX_PEERS; i++) {
        swmp_lobby_poll(&lobbies[i], 3);
        CHECK(!lobbies[i].joining && lobbies[i].session.state == SWMP_SESSION_OFFLINE);
        SwmpRemoteRoster offline = swmp_session_remote_roster(&lobbies[i].session);
        for (uint8_t j = 0; j < SWMP_MAX_PEERS; j++) CHECK(!offline.generation[j]);
    }
    swmp_mock_destroy(mock);
}

static void lobby_edge_test(void) {
    SwmpMock *mock = swmp_mock_create();
    CHECK(mock);
    if (!mock) return;
    SwmpLobby host, client;
    swmp_lobby_init(&host, swmp_mock_transport(mock, 0));
    swmp_lobby_init(&client, swmp_mock_transport(mock, 1));
    CHECK(swmp_lobby_host(&host, &local_build, "Host", NULL, 42));
    swmp_lobby_poll(&host, 0);
    CHECK(swmp_lobby_join(&client, &local_build, "Client", NULL, host.invite, 0));
    swmp_lobby_poll(&client, SWMP_LOBBY_TIMEOUT_MS);
    CHECK(!client.joining && strstr(client.error, "timed out"));
    CHECK(swmp_lobby_join(&client, &local_build, "Client", NULL, host.invite, SWMP_LOBBY_TIMEOUT_MS));
    swmp_lobby_leave(&client);
    swmp_lobby_poll(&host, SWMP_LOBBY_TIMEOUT_MS);
    CHECK(swmp_session_peer_count(&host.session) == 1);

    SwmpTransport raw = swmp_mock_transport(mock, 2);
    SwmpTransportEvent event = {0};
    CHECK(raw.start_join(raw.context, NULL, host.invite, NULL, 0));
    CHECK(raw.poll(raw.context, &event) && event.type == SWMP_TRANSPORT_PEER_CONNECTED);
    SwmpConnection expired = event.connection;
    swmp_lobby_poll(&host, 6000);
    swmp_lobby_poll(&host, 6000 + SWMP_LOBBY_TIMEOUT_MS);
    CHECK(raw.poll(raw.context, &event) && event.type == SWMP_TRANSPORT_PEER_DISCONNECTED);
    raw.stop(raw.context);

    CHECK(raw.start_join(raw.context, NULL, host.invite, NULL, 0));
    CHECK(raw.poll(raw.context, &event) && event.type == SWMP_TRANSPORT_PEER_CONNECTED);
    SwmpConnection connection = event.connection;
    CHECK(connection != expired);
    SwmpControl hello = {.hello.compatibility = local_build, .hello.name = "Raw Link"};
    memcpy(hello.hello.binding, event.binding, SWMP_BINDING_SIZE);
    CHECK(raw_control(&raw, connection, SWMP_MSG_HELLO, 1, 0, 0, &hello));  // spoof before assignment
    swmp_lobby_poll(&host, 12000);
    CHECK(swmp_session_peer_count(&host.session) == 1 && !raw.poll(raw.context, &event));
    CHECK(raw_control(&raw, connection, SWMP_MSG_HELLO, SWMP_UNASSIGNED_PEER, 0, 0, &hello));
    CHECK(raw_control(&raw, connection, SWMP_MSG_HELLO, SWMP_UNASSIGNED_PEER, 0, 0, &hello));  // replay
    swmp_lobby_poll(&host, 12000);
    CHECK(swmp_session_peer_count(&host.session) == 2);
    SwmpPacketHeader h;
    CHECK(raw.poll(raw.context, &event) && swmp_packet_decode(&h, event.data, event.size) == SWMP_PACKET_OK &&
          h.type == SWMP_MSG_WELCOME);
    CHECK(!raw.poll(raw.context, &event));

    CHECK(swmp_lobby_join(&client, &local_build, "Client", NULL, host.invite, 12000));
    swmp_lobby_poll(&client, 12000);
    swmp_lobby_poll(&host, 12000);
    swmp_lobby_poll(&client, 12000);
    CHECK(client.session.local_peer == 2);
    CHECK(raw.poll(raw.context, &event) && swmp_packet_decode(&h, event.data, event.size) == SWMP_PACKET_OK &&
          h.type == SWMP_MSG_PEER_JOINED);

    SwmpControl ping = {.ping_token = UINT64_C(0x8765432112345678)};
    SwmpControl left = {.peer.id = 2};
    CHECK(raw_control(&raw, connection, SWMP_MSG_PING, 2, 42, 1, &ping));  // spoof a different live client
    CHECK(raw_control(&raw, connection, SWMP_MSG_PEER_LEFT, 1, 42, 1, &left));  // impersonate host authority
    CHECK(raw_control(&raw, connection, SWMP_MSG_PING, 1, 43, 1, &ping));  // wrong session
    swmp_lobby_poll(&host, 12000);
    CHECK(!raw.poll(raw.context, &event) && swmp_session_peer(&host.session, 2));
    CHECK(raw_control(&raw, connection, SWMP_MSG_PING, 1, 42, 1, &ping));
    CHECK(raw_control(&raw, connection, SWMP_MSG_PING, 1, 42, 1, &ping));
    swmp_lobby_poll(&host, 12000);
    SwmpControl pong;
    CHECK(raw.poll(raw.context, &event) && swmp_packet_decode(&h, event.data, event.size) == SWMP_PACKET_OK &&
          h.type == SWMP_MSG_PONG &&
          swmp_control_decode(&pong, SWMP_MSG_PONG, event.data + SWMP_PACKET_HEADER_SIZE, h.payload_size) &&
          pong.ping_token == ping.ping_token);
    CHECK(!raw.poll(raw.context, &event));
    raw.stop(raw.context);
    swmp_lobby_leave(&client);
    swmp_lobby_poll(&host, 12001);

    CHECK(raw.start_join(raw.context, NULL, host.invite, NULL, 0));
    CHECK(raw.poll(raw.context, &event) && event.type == SWMP_TRANSPORT_PEER_CONNECTED);
    // The old HELLO payload carries the previous connection binding.
    CHECK(raw_control(&raw, event.connection, SWMP_MSG_HELLO, SWMP_UNASSIGNED_PEER, 0, 0, &hello));
    swmp_lobby_poll(&host, 12001);
    CHECK(raw.poll(raw.context, &event) && swmp_packet_decode(&h, event.data, event.size) == SWMP_PACKET_OK &&
          h.type == SWMP_MSG_REJECT &&
          swmp_control_decode(&pong, SWMP_MSG_REJECT, event.data + SWMP_PACKET_HEADER_SIZE, h.payload_size) &&
          pong.reject.code == SWMP_REJECT_INVALID);
    CHECK(swmp_session_peer_count(&host.session) == 1);
    swmp_mock_destroy(mock);
}

static void lobby_backpressure_test(void) {
    SwmpMock *mock = swmp_mock_create();
    CHECK(mock);
    if (!mock) return;
    SwmpLobby host, client;
    swmp_lobby_init(&host, swmp_mock_transport(mock, 0));
    SwmpTransport t = swmp_mock_transport(mock, 1);
    swmp_lobby_init(&client, t);
    CHECK(swmp_lobby_host(&host, &local_build, "Host", NULL, 95));
    swmp_lobby_poll(&host, 0);

    SwmpTransportEvent event = {0};
    CHECK(t.start_join(t.context, NULL, host.invite, NULL, 0));
    CHECK(t.poll(t.context, &event) && event.type == SWMP_TRANSPORT_PEER_CONNECTED);
    SwmpConnection id = event.connection;
    const uint8_t byte = 1;
    for (unsigned i = 0; i < SWMP_MOCK_QUEUE_SIZE; i++)
        CHECK(host.transport.send(host.transport.context, id, SWMP_CHANNEL_CONTROL, &byte, 1));
    SwmpControl hello = {.hello.compatibility = local_build, .hello.name = "Client"};
    memcpy(hello.hello.binding, event.binding, SWMP_BINDING_SIZE);
    CHECK(raw_control(&t, id, SWMP_MSG_HELLO, SWMP_UNASSIGNED_PEER, 0, 0, &hello));
    swmp_lobby_poll(&host, 1);
    CHECK(swmp_session_peer_count(&host.session) == 1);  // WELCOME could not be queued
    CHECK(t.poll(t.context, &event) && event.type == SWMP_TRANSPORT_PEER_DISCONNECTED);
    t.stop(t.context);

    CHECK(swmp_lobby_join(&client, &local_build, "Client", NULL, host.invite, 2));
    swmp_lobby_poll(&client, 2);
    swmp_lobby_poll(&host, 2);
    swmp_lobby_poll(&client, 2);
    CHECK(client.session.state == SWMP_SESSION_LOBBY && client.session.local_peer == 1);
    id = client.links[0].connection;
    for (unsigned i = 0; i < SWMP_MOCK_QUEUE_SIZE; i++)
        CHECK(host.transport.send(host.transport.context, id, SWMP_CHANNEL_CONTROL, &byte, 1));
    CHECK(swmp_lobby_start(&host));
    swmp_lobby_poll(&host, 3);
    swmp_lobby_poll(&client, 3);
    CHECK(host.session.state == SWMP_SESSION_PLAYING && swmp_session_peer_count(&host.session) == 1);
    CHECK(client.session.state == SWMP_SESSION_OFFLINE);
    swmp_mock_destroy(mock);
}

static void mock_test(void) {
    SwmpMock *mock = swmp_mock_create();
    CHECK(mock);
    if (!mock) return;
    SwmpTransport host = swmp_mock_transport(mock, 0);
    SwmpTransport client = swmp_mock_transport(mock, 1);
    SwmpTransportEvent event = {0};
    CHECK(host.start_host(host.context, NULL, NULL, 0));
    CHECK(host.poll(host.context, &event) && event.type == SWMP_TRANSPORT_INVITE_READY);
    CHECK(client.start_join(client.context, NULL, event.text, NULL, 0));
    CHECK(host.poll(host.context, &event) && event.type == SWMP_TRANSPORT_PEER_CONNECTED);
    SwmpConnection id = event.connection;
    CHECK(client.poll(client.context, &event) && event.type == SWMP_TRANSPORT_PEER_CONNECTED);
    uint8_t byte = 1;
    swmp_mock_fault(mock, 0, SWMP_MOCK_DROP_STATE);
    CHECK(host.send(host.context, id, SWMP_CHANNEL_STATE, &byte, 1));
    CHECK(!client.poll(client.context, &event));
    CHECK(host.send(host.context, id, SWMP_CHANNEL_STATE, &byte, 1));
    byte = 2;
    swmp_mock_fault(mock, 0, SWMP_MOCK_REORDER_STATE);
    CHECK(host.send(host.context, id, SWMP_CHANNEL_STATE, &byte, 1));
    byte = 3;
    swmp_mock_fault(mock, 0, SWMP_MOCK_DUPLICATE_STATE);
    CHECK(host.send(host.context, id, SWMP_CHANNEL_STATE, &byte, 1));
    const uint8_t expected[] = {2, 1, 3, 3};
    for (size_t i = 0; i < sizeof expected; i++)
        CHECK(client.poll(client.context, &event) && event.type == SWMP_TRANSPORT_PACKET && event.size == 1 &&
              event.data[0] == expected[i]);
    CHECK(!client.poll(client.context, &event));

    for (unsigned i = 0; i < SWMP_MOCK_QUEUE_SIZE; i++)
        CHECK(host.send(host.context, id, SWMP_CHANNEL_STATE, &byte, 1));
    for (unsigned i = 0; i < SWMP_MOCK_QUEUE_SIZE; i++) {
        byte = (uint8_t)i;
        CHECK(host.send(host.context, id, SWMP_CHANNEL_CONTROL, &byte, 1));  // evicts queued state
    }
    CHECK(!host.send(host.context, id, SWMP_CHANNEL_CONTROL, &byte, 1));
    for (unsigned i = 0; i < SWMP_MOCK_QUEUE_SIZE; i++)
        CHECK(client.poll(client.context, &event) && event.type == SWMP_TRANSPORT_PACKET &&
              event.channel == SWMP_CHANNEL_CONTROL && event.size == 1 && event.data[0] == i);
    CHECK(!client.poll(client.context, &event));

    CHECK(host.send(host.context, id, SWMP_CHANNEL_CONTROL, &byte, 1));
    host.disconnect(host.context, id);
    CHECK(client.poll(client.context, &event) && event.type == SWMP_TRANSPORT_PEER_DISCONNECTED);
    CHECK(host.poll(host.context, &event) && event.type == SWMP_TRANSPORT_PEER_DISCONNECTED);
    CHECK(!client.poll(client.context, &event) && !host.send(host.context, id, SWMP_CHANNEL_STATE, &byte, 1));
    swmp_mock_destroy(mock);
}

int main(void) {
    packet_test();
    snapshot_test();
    host_session_test();
    client_session_test();
    control_test();
    lobby_test();
    lobby_edge_test();
    lobby_backpressure_test();
    mock_test();
    if (failures) {
        fprintf(stderr, "multiplayer core: %d failure(s)\n", failures);
        return 1;
    }
    puts("multiplayer core: ok");
    return 0;
}

