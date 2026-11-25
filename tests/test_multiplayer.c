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
        0x53, 0x57, 0x4d, 0x50, 0, 6, 16, 4, 0, 0, 0, 3,
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
    packet[5] = 3;
    CHECK(swmp_packet_decode(&out, packet, sizeof packet) == SWMP_PACKET_BAD_VERSION);
    packet[5] = SWMP_PROTOCOL_VERSION;
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
        .flags = SWMP_PLAYER_GROUNDED | SWMP_PLAYER_GUARDING | SWMP_PLAYER_WEAPON_POSE |
                 SWMP_PLAYER_SHIELD_POSE | SWMP_PLAYER_BOW_POSE,
        .action = 0x1234,
        .area_id = 0x89abcdef,
        .phase_id = 42,
        .position_mm = {INT32_MIN, -123456, INT32_MAX},
        .rotation = {INT16_MIN, -12, INT16_MAX},
        .velocity_cm_s = {-3000, 0, 3000},
        .health_q4 = 120,
        .stamina_q4 = 400,
        .equipment_revision = 0xfedcba98,
        .gear_pose = {
            [SWMP_GEAR_WEAPON] = {.offset_mm = {-750, 1400, 900}, .rotation = {INT16_MIN, 26728, INT16_MAX}},
            [SWMP_GEAR_SHIELD] = {.offset_mm = {130, 1030, -650}, .rotation = {-1000, 0, 1000}},
            [SWMP_GEAR_BOW] = {.offset_mm = {3000, -3000, 26}, .rotation = {300, 40, -600}},
        },
        .nocked_arrow = SWMP_ARROW_NORMAL,
        .bow_animation = SWMP_BOW_ANIM_DRAW,
        .animation_activation = 37,
        .animation_phase = 128,
        .sampled_ms = UINT32_C(0x12345678),
        .arrow_pose = {.offset_mm = {-280, 1280, 450}, .rotation = {-649, -3610, 16003}},
    };
    uint8_t data[SWMP_PLAYER_SNAPSHOT_SIZE];
    CHECK(swmp_player_snapshot_encode(data, &in));
    SwmpPlayerSnapshot out = {};
    CHECK(swmp_player_snapshot_decode(&out, data));
    CHECK(out.peer_id == in.peer_id && out.flags == in.flags && out.action == in.action);
    CHECK(out.area_id == in.area_id && out.phase_id == in.phase_id);
    CHECK(out.sampled_ms == in.sampled_ms);
    CHECK(out.animation_activation == in.animation_activation && out.animation_phase == in.animation_phase);
    CHECK(data[82] == 37 && data[83] == 128 && data[96] == 0x12 && data[99] == 0x78);
    CHECK(!memcmp(out.position_mm, in.position_mm, sizeof in.position_mm));
    CHECK(!memcmp(out.rotation, in.rotation, sizeof in.rotation));
    CHECK(!memcmp(out.velocity_cm_s, in.velocity_cm_s, sizeof in.velocity_cm_s));
    CHECK(out.health_q4 == in.health_q4 && out.stamina_q4 == in.stamina_q4);
    CHECK(out.equipment_revision == in.equipment_revision);
    CHECK(out.nocked_arrow == in.nocked_arrow);
    CHECK(data[81] == SWMP_BOW_ANIM_DRAW && out.bow_animation == in.bow_animation);
    CHECK(!memcmp(out.arrow_pose.offset_mm, in.arrow_pose.offset_mm, sizeof in.arrow_pose.offset_mm));
    CHECK(!memcmp(out.arrow_pose.rotation, in.arrow_pose.rotation, sizeof in.arrow_pose.rotation));
    for (unsigned i = 0; i < SWMP_GEAR_SLOTS; i++) {
        CHECK(!memcmp(out.gear_pose[i].offset_mm, in.gear_pose[i].offset_mm, sizeof in.gear_pose[i].offset_mm));
        CHECK(!memcmp(out.gear_pose[i].rotation, in.gear_pose[i].rotation, sizeof in.gear_pose[i].rotation));
    }
    for (uint8_t arrow = SWMP_ARROW_FIRE; arrow < SWMP_ARROW_COUNT; arrow++) {
        in.nocked_arrow = arrow;
        CHECK(swmp_player_snapshot_encode(data, &in));
        CHECK(data[80] == arrow);
        CHECK(swmp_player_snapshot_decode(&out, data));
        CHECK(out.nocked_arrow == arrow && out.arrow_pose.offset_mm[0] == -280);
    }
    in.nocked_arrow = SWMP_ARROW_COUNT;
    CHECK(!swmp_player_snapshot_encode(data, &in));
    in.nocked_arrow = SWMP_ARROW_NORMAL;
    CHECK(swmp_player_snapshot_encode(data, &in));
    data[68] = 0x7f;
    CHECK(!swmp_player_snapshot_decode(&out, data));
    CHECK(swmp_player_snapshot_encode(data, &in));
    data[80] = 255;
    CHECK(!swmp_player_snapshot_decode(&out, data));
    CHECK(swmp_player_snapshot_encode(data, &in));
    data[81] = SWMP_BOW_ANIM_COUNT;
    CHECK(!swmp_player_snapshot_decode(&out, data));
    CHECK(swmp_player_snapshot_encode(data, &in));
    data[82] = 0;  // a phase requires an activation
    CHECK(!swmp_player_snapshot_decode(&out, data));
    in.animation_activation = 0;
    CHECK(!swmp_player_snapshot_encode(data, &in));
    in.animation_phase = 0;
    CHECK(swmp_player_snapshot_encode(data, &in));
    data[84] = 0x7f;
    CHECK(!swmp_player_snapshot_decode(&out, data));
    CHECK(swmp_player_snapshot_encode(data, &in));
    data[1] &= ~SWMP_PLAYER_BOW_POSE;
    data[81] = SWMP_BOW_ANIM_NONE;
    CHECK(!swmp_player_snapshot_decode(&out, data));
    data[80] = SWMP_ARROW_NONE;
    data[81] = SWMP_BOW_ANIM_DRAW;
    CHECK(!swmp_player_snapshot_decode(&out, data));
    CHECK(swmp_player_snapshot_encode(data, &in));
    data[1] = SWMP_PLAYER_BOW_POSE;
    memset(data + 40, 0, 4);
    CHECK(!swmp_player_snapshot_decode(&out, data));
    in.equipment_revision = 0;
    CHECK(!swmp_player_snapshot_encode(data, &in));
    in.equipment_revision = 1;
    in.nocked_arrow = SWMP_ARROW_NONE;
    in.bow_animation = SWMP_BOW_ANIM_COUNT;
    CHECK(!swmp_player_snapshot_encode(data, &in));
    in.bow_animation = SWMP_BOW_ANIM_DRAW;
    in.flags &= ~SWMP_PLAYER_BOW_POSE;
    CHECK(!swmp_player_snapshot_encode(data, &in));
    in.bow_animation = SWMP_BOW_ANIM_NONE;
    CHECK(swmp_player_snapshot_encode(data, &in));
    CHECK(!memcmp(data + 80, (const uint8_t[16]){0}, 16));
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
    CHECK(size == 96 && data[1] == SWMP_PROTOCOL_VERSION && data[14] == 0x71 && data[34] == 0x92 && data[89] == 0xa5);
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
    in = (SwmpControl){.equipment = {.peer = 3, .state = {
        .gear = {.id = {SWMP_GEAR_WOODCUTTERS_AXE, SWMP_GEAR_WOODEN_SHIELD, SWMP_GEAR_TRAVELERS_BOW},
                 .armor = {UINT32_C(0x10203040), UINT32_C(0x50607080), 0}}, .revision = 8}}};
    CHECK(swmp_control_encode(data, sizeof data, &size, SWMP_MSG_EQUIPMENT, &in));
    CHECK(swmp_control_decode(&out, SWMP_MSG_EQUIPMENT, data, size));
    CHECK(size == 20 && out.equipment.peer == 3 && out.equipment.state.revision == 8);
    CHECK(!memcmp(out.equipment.state.gear.id, in.equipment.state.gear.id, SWMP_GEAR_SLOTS));
    CHECK(!memcmp(out.equipment.state.gear.armor, in.equipment.state.gear.armor, sizeof in.equipment.state.gear.armor));
    CHECK(data[8] == 0x10 && data[15] == 0x80);
    CHECK(!swmp_control_decode(&out, SWMP_MSG_EQUIPMENT, data, size - 1));
    data[7] = SWMP_GEAR_WOODEN_SHIELD;
    CHECK(!swmp_control_decode(&out, SWMP_MSG_EQUIPMENT, data, size));
    in.equipment.state.gear.id[SWMP_GEAR_SHIELD] = SWMP_GEAR_SOLDIERS_SHIELD;
    in.equipment.state.gear.id[SWMP_GEAR_BOW] = SWMP_GEAR_SOLDIERS_BOW;
    CHECK(swmp_control_encode(data, sizeof data, &size, SWMP_MSG_EQUIPMENT, &in));
    CHECK(swmp_control_decode(&out, SWMP_MSG_EQUIPMENT, data, size));
    CHECK(!memcmp(out.equipment.state.gear.id, in.equipment.state.gear.id, SWMP_GEAR_SLOTS));
    data[6] = SWMP_GEAR_SOLDIERS_BOW;
    CHECK(!swmp_control_decode(&out, SWMP_MSG_EQUIPMENT, data, size));
    data[6] = SWMP_GEAR_SOLDIERS_SHIELD;
    data[7] = SWMP_GEAR_SOLDIERS_SHIELD;
    CHECK(!swmp_control_decode(&out, SWMP_MSG_EQUIPMENT, data, size));
    const SwmpGear weapons[] = {
        SWMP_GEAR_TRAVELERS_SWORD, SWMP_GEAR_TRAVELERS_SPEAR, SWMP_GEAR_TRAVELERS_CLAYMORE,
    };
    for (unsigned i = 0; i < sizeof weapons / sizeof *weapons; i++) {
        in.equipment.state.gear.id[SWMP_GEAR_WEAPON] = weapons[i];
        CHECK(swmp_control_encode(data, sizeof data, &size, SWMP_MSG_EQUIPMENT, &in));
        CHECK(swmp_control_decode(&out, SWMP_MSG_EQUIPMENT, data, size));
        CHECK(!memcmp(out.equipment.state.gear.id, in.equipment.state.gear.id, SWMP_GEAR_SLOTS));
        data[6] = weapons[i];
        CHECK(!swmp_control_decode(&out, SWMP_MSG_EQUIPMENT, data, size));
    }
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

static unsigned host_joins, observer_joins, host_leaves, observer_leaves;
static char last_join[SWMP_PLAYER_NAME_SIZE];
static char last_host_leave[SWMP_PLAYER_NAME_SIZE], last_observer_leave[SWMP_PLAYER_NAME_SIZE];

static void host_peer_changed(const char *name, bool joined, uint64_t now) {
    (void)now;
    if (joined) {
        host_joins++;
        strcpy(last_join, name);
    } else {
        host_leaves++;
        strcpy(last_host_leave, name);
    }
}

static void observer_peer_changed(const char *name, bool joined, uint64_t now) {
    (void)now;
    CHECK(swmp_player_name_valid(name));
    if (joined) {
        observer_joins++;
    } else {
        observer_leaves++;
        strcpy(last_observer_leave, name);
    }
}

static void lobby_test(void) {
    SwmpMock *mock = swmp_mock_create();
    CHECK(mock);
    if (!mock) return;
    SwmpLobby lobbies[SWMP_MAX_PEERS + 1];
    for (unsigned i = 0; i <= SWMP_MAX_PEERS; i++)
        swmp_lobby_init(&lobbies[i], swmp_mock_transport(mock, i));
    SwmpLobby *host = &lobbies[0], *extra = &lobbies[SWMP_MAX_PEERS];
    host->peer_changed = host_peer_changed;
    lobbies[2].peer_changed = observer_peer_changed;
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
    CHECK(host_joins == 7 && observer_joins == 5 && !strcmp(last_join, "Link 7"));
    SwmpGearSet with_bow = {.id = {SWMP_GEAR_WOODCUTTERS_AXE, SWMP_GEAR_WOODEN_SHIELD, SWMP_GEAR_TRAVELERS_BOW}};
    SwmpGearSet with_spear = {.id = {SWMP_GEAR_TRAVELERS_SPEAR, SWMP_GEAR_WOODEN_SHIELD, SWMP_GEAR_TRAVELERS_BOW}};
    SwmpGearSet without_bow = {.id = {SWMP_GEAR_TRAVELERS_SPEAR, SWMP_GEAR_WOODEN_SHIELD}};
    SwmpGearSet axe = {.id = {SWMP_GEAR_WOODCUTTERS_AXE}};
    swmp_lobby_set_equipment(&lobbies[1], with_bow);
    swmp_lobby_set_equipment(&lobbies[1], with_bow);
    swmp_lobby_set_equipment(&lobbies[3], axe);
    SwmpControl forged = {.equipment = {.peer = 3, .state = {.revision = 99}}};
    CHECK(raw_control(&lobbies[1].transport, lobbies[1].links[0].connection, SWMP_MSG_EQUIPMENT, 1,
                      host->session.session_id, 100, &forged));
    // Position updates work in an open lobby, without START, and the host relays every peer in a batch.
    SwmpPlayerSnapshot position = {.area_id = 1, .gear_pose = {
        [SWMP_GEAR_WEAPON].offset_mm = {250, 1420},
        [SWMP_GEAR_SHIELD].offset_mm = {0, 1300, -350},
        [SWMP_GEAR_BOW].offset_mm = {100, 1200, -400},
    }, .arrow_pose.offset_mm = {-280, 1280, 450}};
    for (uint8_t i = 1; i < SWMP_MAX_PEERS; i++) {
        position.flags = i == 1 ? SWMP_PLAYER_WEAPON_POSE | SWMP_PLAYER_SHIELD_POSE | SWMP_PLAYER_BOW_POSE :
            i == 3 ? SWMP_PLAYER_WEAPON_POSE : 0;
        position.nocked_arrow = i == 1 ? SWMP_ARROW_ANCIENT : SWMP_ARROW_NONE;
        position.bow_animation = i == 1 ? SWMP_BOW_ANIM_DRAW : SWMP_BOW_ANIM_NONE;
        position.sampled_ms = UINT32_MAX - i;
        position.animation_activation = i;
        position.animation_phase = 43;
        position.position_mm[0] = i * 1000;
        swmp_lobby_send_position(&lobbies[i], &position, 1);
    }
    swmp_lobby_poll(host, 1);
    position.position_mm[0] = 10;
    position.sampled_ms = 42;
    position.animation_activation = position.animation_phase = 0;
    SwmpEnvironment environment = {.time_milli = 359999, .weather = 2};
    swmp_lobby_send_state(host, &position, &environment, 1);
    swmp_lobby_poll(&lobbies[2], 1);
    CHECK(lobbies[2].environment.area_id == 1 && lobbies[2].environment.received_at == 1);
    CHECK(lobbies[2].environment.state.time_milli == 359999 && lobbies[2].environment.state.weather == 2);
    CHECK(lobbies[2].equipment[1].revision == 1 && lobbies[2].equipment[3].revision == 1);
    CHECK(lobbies[2].equipment[3].gear.id[SWMP_GEAR_WEAPON] == SWMP_GEAR_WOODCUTTERS_AXE);
    CHECK(lobbies[2].positions[1].snapshot.equipment_revision == lobbies[2].equipment[1].revision);
    CHECK(lobbies[2].positions[1].snapshot.gear_pose[SWMP_GEAR_WEAPON].offset_mm[1] == 1420);
    CHECK(lobbies[2].positions[1].snapshot.gear_pose[SWMP_GEAR_SHIELD].offset_mm[2] == -350);
    CHECK(lobbies[2].positions[1].snapshot.gear_pose[SWMP_GEAR_BOW].offset_mm[2] == -400);
    CHECK(lobbies[2].positions[1].snapshot.nocked_arrow == SWMP_ARROW_ANCIENT);
    CHECK(lobbies[2].positions[1].snapshot.bow_animation == SWMP_BOW_ANIM_DRAW);
    CHECK(lobbies[2].positions[1].snapshot.arrow_pose.offset_mm[2] == 450);
    CHECK(lobbies[2].positions[3].snapshot.nocked_arrow == SWMP_ARROW_NONE);
    for (uint8_t i = 0; i < SWMP_MAX_PEERS; i++) {
        if (i == 2) continue;
        CHECK(swmp_position_fresh(&lobbies[2].positions[i], 1));
        CHECK(lobbies[2].positions[i].snapshot.position_mm[0] == (i ? i * 1000 : 10));
        CHECK(lobbies[2].positions[i].snapshot.sampled_ms == (i ? UINT32_MAX - i : 42));
        CHECK(lobbies[2].positions[i].snapshot.animation_activation == i);
        CHECK(lobbies[2].positions[i].snapshot.animation_phase == (i ? 43 : 0));
    }
    CHECK(swmp_lobby_start(host));
    uint32_t old_epoch = host->roster_epoch;

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
    CHECK(host_joins == 8 && observer_joins == 6 && !strcmp(last_join, "Link 3"));
    CHECK(host_leaves == 1 && observer_leaves == 1);
    CHECK(!strcmp(last_host_leave, "Link 3") && !strcmp(last_observer_leave, "Link 3"));
    SwmpRemoteRoster host_after = swmp_session_remote_roster(&host->session);
    SwmpRemoteRoster client_after = swmp_session_remote_roster(&lobbies[2].session);
    CHECK(host_after.generation[3] > host_before.generation[3]);
    CHECK(client_after.generation[3] > client_before.generation[3]);
    CHECK(host_after.generation[1] == host_before.generation[1]);
    CHECK(client_after.generation[0] == client_before.generation[0]);
    CHECK(!host->positions[3].snapshot.area_id && !lobbies[2].positions[3].snapshot.area_id);
    CHECK(!host->equipment[3].revision && !lobbies[2].equipment[3].revision && !extra->equipment[3].revision);
    CHECK(extra->equipment[1].revision == 1 && extra->equipment[1].gear.id[SWMP_GEAR_BOW] == SWMP_GEAR_TRAVELERS_BOW);
    swmp_lobby_set_equipment(&lobbies[1], with_spear);
    swmp_lobby_poll(host, 2);
    swmp_lobby_poll(extra, 2);
    CHECK(extra->equipment[1].revision == 2);
    CHECK(extra->equipment[1].gear.id[SWMP_GEAR_WEAPON] == SWMP_GEAR_TRAVELERS_SPEAR);
    CHECK(extra->equipment[1].gear.id[SWMP_GEAR_SHIELD] == SWMP_GEAR_WOODEN_SHIELD);
    CHECK(extra->equipment[1].gear.id[SWMP_GEAR_BOW] == SWMP_GEAR_TRAVELERS_BOW);
    swmp_lobby_set_equipment(&lobbies[1], without_bow);
    swmp_lobby_poll(host, 2);
    swmp_lobby_poll(extra, 2);
    CHECK(extra->equipment[1].revision == 3 && extra->equipment[1].gear.id[SWMP_GEAR_BOW] == SWMP_GEAR_NONE);
    CHECK(extra->equipment[1].gear.id[SWMP_GEAR_SHIELD] == SWMP_GEAR_WOODEN_SHIELD);
    CHECK(extra->equipment[1].gear.id[SWMP_GEAR_WEAPON] == SWMP_GEAR_TRAVELERS_SPEAR);

    // A host state datagram from before the replacement cannot populate the reused slot.
    uint8_t packet[SWMP_PACKET_HEADER_SIZE + SWMP_ENVIRONMENT_SIZE + 2 * SWMP_PLAYER_SNAPSHOT_SIZE];
    SwmpPacketHeader state = {
        .version = SWMP_PROTOCOL_VERSION, .type = SWMP_MSG_PLAYER_SNAPSHOT,
        .payload_size = SWMP_ENVIRONMENT_SIZE + 2 * SWMP_PLAYER_SNAPSHOT_SIZE, .session_id = host->session.session_id,
        .sender_peer = SWMP_HOST_PEER, .channel = SWMP_CHANNEL_STATE, .sequence = 2, .tick = old_epoch,
    };
    position.peer_id = SWMP_HOST_PEER;
    environment.weather = 6;
    CHECK(swmp_environment_encode(packet + SWMP_PACKET_HEADER_SIZE, &environment));
    CHECK(swmp_player_snapshot_encode(packet + SWMP_PACKET_HEADER_SIZE + SWMP_ENVIRONMENT_SIZE, &position));
    position.peer_id = 3;
    CHECK(swmp_player_snapshot_encode(packet + SWMP_PACKET_HEADER_SIZE + SWMP_ENVIRONMENT_SIZE +
                                      SWMP_PLAYER_SNAPSHOT_SIZE, &position));
    CHECK(swmp_packet_encode(packet, sizeof packet, &state) == SWMP_PACKET_OK);
    CHECK(host->transport.send(host->transport.context, lobbies[2].links[0].connection,
                              SWMP_CHANNEL_STATE, packet, sizeof packet));
    swmp_lobby_poll(&lobbies[2], 2);
    CHECK(!lobbies[2].positions[3].snapshot.area_id);
    CHECK(lobbies[2].environment.state.weather == 2);

    // A client cannot supply an environment even with an authenticated sender and valid payload.
    state.sender_peer = 3;
    state.tick = 0;
    state.payload_size -= SWMP_PLAYER_SNAPSHOT_SIZE;
    CHECK(swmp_player_snapshot_encode(packet + SWMP_PACKET_HEADER_SIZE + SWMP_ENVIRONMENT_SIZE, &position));
    CHECK(swmp_packet_encode(packet, sizeof packet, &state) == SWMP_PACKET_OK);
    CHECK(extra->transport.send(extra->transport.context, extra->links[0].connection,
                                SWMP_CHANNEL_STATE, packet, SWMP_PACKET_HEADER_SIZE + state.payload_size));
    swmp_lobby_poll(host, 2);
    CHECK(!host->environment.area_id && !host->positions[3].snapshot.area_id);

    // A late join sees current players with the next batch, and expired entries drop out.
    SwmpPlayerSnapshot before_unequip = position;
    before_unequip.flags = SWMP_PLAYER_BOW_POSE;
    before_unequip.nocked_arrow = SWMP_ARROW_NORMAL;
    before_unequip.bow_animation = SWMP_BOW_ANIM_DRAW;
    swmp_lobby_send_position(&lobbies[1], &before_unequip, 51);
    swmp_lobby_poll(host, 51);
    environment.time_milli = 0;
    swmp_lobby_send_state(host, &position, &environment, 51);
    swmp_lobby_poll(extra, 51);
    CHECK(extra->environment.area_id == 1 && extra->environment.state.time_milli == 0);
    CHECK(extra->environment.state.weather == 6);
    const SwmpPlayerSnapshot *after_unequip = &extra->positions[1].snapshot;
    CHECK(after_unequip->equipment_revision == 3 && !(after_unequip->flags & SWMP_PLAYER_BOW_POSE));
    CHECK(after_unequip->nocked_arrow == SWMP_ARROW_NONE && after_unequip->bow_animation == SWMP_BOW_ANIM_NONE);
    CHECK(extra->positions[2].snapshot.position_mm[0] == 2000);
    CHECK(!swmp_position_fresh(&extra->positions[2], 1051));
    swmp_lobby_send_position(host, &position, 1051);
    swmp_lobby_poll(extra, 1051);
    CHECK(!extra->positions[2].snapshot.area_id);
    CHECK(!extra->environment.area_id);

    // A client snapshot can be lost or delayed. A newer action must survive both cases.
    position.area_id = 2;
    position.action = 1;
    swmp_mock_fault(mock, SWMP_MAX_PEERS, SWMP_MOCK_DROP_STATE);
    swmp_lobby_send_position(extra, &position, 1101);
    swmp_lobby_poll(host, 1101);
    CHECK(!host->positions[3].snapshot.area_id);
    swmp_lobby_send_position(extra, &position, 1151);
    position.area_id = 3;
    position.action = 2;
    position.health_q4 = 9;
    position.rotation[1] = -16384;
    position.position_mm[0] = -24000;
    swmp_mock_fault(mock, SWMP_MAX_PEERS, SWMP_MOCK_REORDER_STATE);
    swmp_lobby_send_position(extra, &position, 1201);
    swmp_lobby_poll(host, 1201);
    CHECK(host->positions[3].snapshot.action == 2);
    swmp_lobby_send_position(host, &position, 1201);
    swmp_lobby_poll(&lobbies[2], 1201);
    const SwmpPlayerSnapshot *latest = &lobbies[2].positions[3].snapshot;
    CHECK(latest->area_id == 3 && latest->action == 2 && latest->health_q4 == 9);
    CHECK(latest->rotation[1] == -16384 && latest->position_mm[0] == -24000);

    // A lost withdrawal must expire even if the host continues batching other players.
    position.area_id = 0;
    swmp_mock_fault(mock, SWMP_MAX_PEERS, SWMP_MOCK_DROP_STATE);
    swmp_lobby_send_position(extra, &position, 1251);
    swmp_lobby_poll(host, 1251);
    position.area_id = 1;
    swmp_lobby_send_position(host, &position, 2251);
    swmp_lobby_poll(&lobbies[2], 2251);
    CHECK(!lobbies[2].positions[3].snapshot.area_id);
    CHECK(lobbies[2].positions[SWMP_HOST_PEER].snapshot.area_id == 1);

    // An armor-only update advances the revision and keeps the rigid equipment intact.
    SwmpGearSet armored = without_bow;
    armored.armor[SWMP_ARMOR_HEAD] = UINT32_C(0x12345678);
    swmp_lobby_set_equipment(&lobbies[1], armored);
    swmp_lobby_poll(host, 2251);
    swmp_lobby_poll(extra, 2251);
    CHECK(extra->equipment[1].revision == 4);
    CHECK(extra->equipment[1].gear.armor[SWMP_ARMOR_HEAD] == armored.armor[SWMP_ARMOR_HEAD]);
    CHECK(!memcmp(extra->equipment[1].gear.id, without_bow.id, sizeof without_bow.id));

    swmp_lobby_leave(host);
    for (unsigned i = 1; i <= SWMP_MAX_PEERS; i++) {
        swmp_lobby_poll(&lobbies[i], 2252);
        CHECK(!lobbies[i].joining && lobbies[i].session.state == SWMP_SESSION_OFFLINE);
        SwmpRemoteRoster offline = swmp_session_remote_roster(&lobbies[i].session);
        for (uint8_t j = 0; j < SWMP_MAX_PEERS; j++) CHECK(!offline.generation[j]);
    }
    CHECK(host_leaves == 1 && observer_leaves == 2 && !strcmp(last_observer_leave, "Host"));
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

