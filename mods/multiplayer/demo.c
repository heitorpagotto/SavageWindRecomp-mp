// In-process lobby handshake and snapshot codec demonstration.
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "lobby.h"
#include "mock_transport.h"

enum { DEMO_TICKS = 10 };

static int fail(const char *what) {
    fprintf(stderr, "multiplayer demo: %s\n", what);
    return 1;
}

static SwmpPlayerSnapshot sample(uint8_t peer, uint32_t tick) {
    SwmpPlayerSnapshot s = {
        .peer_id = peer,
        .flags = SWMP_PLAYER_GROUNDED,
        .action = (uint16_t)(tick % 4),
        .area_id = 1,
        .phase_id = 1,
        .position_mm = {(int32_t)peer * 1500 + (int32_t)tick * 75, 2000, (int32_t)peer * -800},
        .rotation = {0, (int16_t)(peer * 2048), 0},
        .velocity_cm_s = {150, 0, 0},
        .health_q4 = 120,
        .stamina_q4 = 400,
        .equipment_revision = 1,
    };
    return s;
}

static bool create_lobby(SwmpSession sessions[SWMP_MAX_PEERS], uint64_t session_id) {
    const SwmpCompatibility compatibility = {
        .protocol = SWMP_PROTOCOL_VERSION, .mod_version = {0, 1, 0}, .game_version = {1, 9, 0},
        .savage_build = {1}, .game_build = {2},  // synthetic fixture IDs
    };
    SwmpMock *mock = swmp_mock_create();
    if (!mock) return false;
    SwmpLobby lobbies[SWMP_MAX_PEERS];
    for (uint8_t i = 0; i < SWMP_MAX_PEERS; i++)
        swmp_lobby_init(&lobbies[i], swmp_mock_transport(mock, i));

    SwmpLobby *host = &lobbies[0];
    bool ok = swmp_lobby_host(host, &compatibility, "Host Link", NULL, session_id);
    swmp_lobby_poll(host, 0);
    for (uint8_t i = 1; i < SWMP_MAX_PEERS && ok; i++) {
        char name[SWMP_PLAYER_NAME_SIZE];
        snprintf(name, sizeof name, "Link %u", (unsigned)i + 1);
        ok = swmp_lobby_join(&lobbies[i], &compatibility, name, NULL, host->invite, 0);
        swmp_lobby_poll(&lobbies[i], 0);
    }
    swmp_lobby_poll(host, 1);
    ok = ok && swmp_session_peer_count(&host->session) == SWMP_MAX_PEERS && swmp_lobby_start(host);
    for (uint8_t i = 0; i < SWMP_MAX_PEERS; i++) {
        swmp_lobby_poll(&lobbies[i], 2);
        sessions[i] = lobbies[i].session;
        ok = ok && sessions[i].state == SWMP_SESSION_PLAYING && sessions[i].local_peer == i &&
             swmp_session_peer_count(&sessions[i]) == SWMP_MAX_PEERS;
    }
    swmp_mock_destroy(mock);
    return ok;
}

int main(void) {
    const uint64_t session_id = UINT64_C(0x53574d5044454d4f);  // SWMPDEMO
    SwmpSession sessions[SWMP_MAX_PEERS];
    if (!create_lobby(sessions, session_id)) return fail("mock lobby handshake failed");

    printf("Savage Wind Multiplayer local demo (protocol %u)\n", SWMP_PROTOCOL_VERSION);
    printf("Lobby ready: %zu/8 Links; host authority at peer 0\n", swmp_session_peer_count(&sessions[0]));
    puts("Lobby transport: deterministic mock. Snapshot deliveries: in-process codec checks.");

    bool seen[SWMP_MAX_PEERS][SWMP_MAX_PEERS] = {{false}};
    SwmpPlayerSnapshot latest[SWMP_MAX_PEERS][SWMP_MAX_PEERS] = {};
    unsigned deliveries = 0, stale_drops = 0;
    for (uint32_t tick = 0; tick < DEMO_TICKS; tick++) {
        for (uint8_t sender = 0; sender < SWMP_MAX_PEERS; sender++) {
            uint8_t packet[SWMP_PACKET_HEADER_SIZE + SWMP_PLAYER_SNAPSHOT_SIZE] = {};
            SwmpPlayerSnapshot state = sample(sender, tick);
            if (!swmp_player_snapshot_encode(packet + SWMP_PACKET_HEADER_SIZE, &state))
                return fail("snapshot encode failed");
            uint32_t sequence;
            if (!swmp_session_next_sequence(&sessions[sender], SWMP_CHANNEL_STATE, &sequence))
                return fail("sequence allocation failed");
            SwmpPacketHeader sent = {
                .version = SWMP_PROTOCOL_VERSION,
                .type = SWMP_MSG_PLAYER_SNAPSHOT,
                .payload_size = SWMP_PLAYER_SNAPSHOT_SIZE,
                .session_id = session_id,
                .sender_peer = sender,
                .channel = SWMP_CHANNEL_STATE,
                .sequence = sequence,
                .tick = tick,
            };
            if (swmp_packet_encode(packet, sizeof packet, &sent) != SWMP_PACKET_OK)
                return fail("packet encode failed");
            SwmpPacketHeader received;
            SwmpPlayerSnapshot decoded;
            if (swmp_packet_decode(&received, packet, sizeof packet) != SWMP_PACKET_OK ||
                !swmp_player_snapshot_decode(&decoded, packet + SWMP_PACKET_HEADER_SIZE))
                return fail("wire round-trip failed");

            for (uint8_t recipient = 0; recipient < SWMP_MAX_PEERS; recipient++) {
                if (recipient == sender) continue;
                if (swmp_session_receive(&sessions[recipient], &received) != SWMP_RECEIVE_ACCEPT)
                    return fail("receiver rejected a fresh snapshot");
                latest[recipient][sender] = decoded;
                seen[recipient][sender] = true;
                deliveries++;
            }
            if (tick == 3 && sender == 4) {
                if (swmp_session_receive(&sessions[0], &received) != SWMP_RECEIVE_DROP_STALE)
                    return fail("duplicate snapshot was not rejected");
                stale_drops++;
            }
        }
        if (tick == 0 || tick == DEMO_TICKS - 1)
            printf("Tick %u: host sees Link 8 at (%d, %d, %d) mm\n", tick,
                   latest[0][7].position_mm[0], latest[0][7].position_mm[1],
                   latest[0][7].position_mm[2]);
    }
    for (uint8_t recipient = 0; recipient < SWMP_MAX_PEERS; recipient++)
        for (uint8_t sender = 0; sender < SWMP_MAX_PEERS; sender++)
            if (recipient != sender && !seen[recipient][sender]) return fail("a Link never saw another Link");

    printf("PASS: 8 Links, %u snapshot deliveries, %u stale duplicate rejected\n", deliveries, stale_drops);
    return 0;
}

