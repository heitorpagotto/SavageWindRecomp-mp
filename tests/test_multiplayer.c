#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "protocol.h"
#include "session.h"

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

int main(void) {
    packet_test();
    snapshot_test();
    host_session_test();
    client_session_test();
    if (failures) {
        fprintf(stderr, "multiplayer core: %d failure(s)\n", failures);
        return 1;
    }
    puts("multiplayer core: ok");
    return 0;
}

