// Exercise HTTPS signaling and WebRTC together without game files.
#define _POSIX_C_SOURCE 200809L

#include "lobby.h"
#include "rendezvous.h"

#undef NDEBUG
#include <assert.h>
#include <curl/curl.h>
#include <rtc/rtc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <time.h>
#endif

static uint64_t now_ms(void) {
#ifdef _WIN32
    return GetTickCount64();
#else
    struct timespec now;
    assert(clock_gettime(CLOCK_MONOTONIC, &now) == 0);
    return (uint64_t)now.tv_sec * 1000 + (uint64_t)now.tv_nsec / 1000000;
#endif
}

static void pause_poll(void) {
#ifdef _WIN32
    Sleep(1);
#else
    nanosleep(&(struct timespec){.tv_nsec = 1000000}, NULL);
#endif
}

typedef struct {
    SwmpRendezvous *signaling;
    SwmpWebrtc *rtc;
    SwmpLobby lobby;
} Peer;

static const char *test_turn;

static void init(Peer *peer, const char *ca) {
    peer->signaling = swmp_rendezvous_create(ca);
    assert(peer->signaling);
    SwmpWebrtcConfig config = {
        .bind_address = "127.0.0.1",
        .ice_servers = &test_turn, .ice_server_count = test_turn ? 1 : 0, .relay_only = test_turn != NULL,
    };
    peer->rtc = swmp_webrtc_create(config, swmp_rendezvous_signaling(peer->signaling));
    assert(peer->rtc);
    swmp_lobby_init(&peer->lobby, swmp_webrtc_transport(peer->rtc));
}

static void tick(Peer peers[], unsigned count) {
    uint64_t now = now_ms();
    for (unsigned i = 0; i < count; i++) {
        swmp_rendezvous_update(peers[i].signaling, now);
        swmp_lobby_poll(&peers[i].lobby, now);
    }
    pause_poll();
}

static void destroy(Peer *peer) {
    swmp_webrtc_destroy(peer->rtc);
    swmp_rendezvous_destroy(peer->signaling);
}

static SwmpConnection peer_connection(const SwmpLobby *lobby, uint8_t peer) {
    for (unsigned i = 0; i < SWMP_LOBBY_LINKS; i++)
        if (lobby->links[i].connection && lobby->links[i].peer == peer) return lobby->links[i].connection;
    return 0;
}

static SwmpCompatibility compatibility = {
    .protocol = SWMP_PROTOCOL_VERSION, .mod_version = {0, 1, 0}, .game_version = {1, 9, 0},
    .savage_build = {1}, .game_build = {2},
};

static void check_untrusted(const char *url) {
    Peer peer = {0};
    init(&peer, NULL);
    assert(!swmp_lobby_host(&peer.lobby, &compatibility, "Host", "http://127.0.0.1", 42));
    assert(swmp_lobby_host(&peer.lobby, &compatibility, "Host", url, 42));
    uint64_t deadline = now_ms() + 10000;
    while (peer.lobby.session.state != SWMP_SESSION_OFFLINE && now_ms() < deadline) tick(&peer, 1);
    assert(peer.lobby.session.state == SWMP_SESSION_OFFLINE && !peer.lobby.invite[0]);
    assert(!strcmp(peer.lobby.error, "Could not reach the HTTPS rendezvous service"));
    destroy(&peer);
}

int main(int argc, char **argv) {
    assert(argc == 3);
    assert(curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK);
    rtcPreload();
    check_untrusted(argv[1]);
    test_turn = getenv("SWMP_TEST_TURN");
    Peer peers[SWMP_MAX_PEERS] = {0};
    for (unsigned i = 0; i < SWMP_MAX_PEERS; i++) init(&peers[i], argv[2]);
    SwmpLobby *host = &peers[0].lobby;
    assert(swmp_lobby_host(host, &compatibility, "Host", argv[1], 42));
    uint64_t deadline = now_ms() + 10000;
    while (!host->invite[0] && !host->error[0] && now_ms() < deadline) tick(peers, SWMP_MAX_PEERS);
    if (host->error[0]) fprintf(stderr, "Host signaling error: %s\n", host->error);
    assert(strlen(host->invite) == 24);
    for (unsigned i = 1; i < SWMP_MAX_PEERS; i++)
        assert(swmp_lobby_join(&peers[i].lobby, &compatibility, "Client", argv[1], host->invite, now_ms()));
    deadline = now_ms() + 25000;
    for (unsigned i = 0; i < SWMP_MAX_PEERS; i++) {
        while (swmp_session_peer_count(&peers[i].lobby.session) < SWMP_MAX_PEERS && now_ms() < deadline)
            tick(peers, SWMP_MAX_PEERS);
        if (swmp_session_peer_count(&peers[i].lobby.session) != SWMP_MAX_PEERS) {
            for (unsigned j = 0; j < SWMP_MAX_PEERS; j++)
                fprintf(stderr, "Peer %u has %zu in roster (joining %d): %s\n", j,
                        swmp_session_peer_count(&peers[j].lobby.session), peers[j].lobby.joining, peers[j].lobby.error);
            assert(!"incomplete HTTPS lobby roster");
        }
    }

    if (test_turn) {
        // A peer-reflexive candidate can describe the other side of a relayed path.
        for (unsigned i = 1; i < SWMP_MAX_PEERS; i++) {
            SwmpConnection host_link = peer_connection(host, peers[i].lobby.session.local_peer);
            SwmpConnection client_link = peer_connection(&peers[i].lobby, SWMP_HOST_PEER);
            assert(host_link && client_link);
            assert(swmp_webrtc_link_uses_relay(peers[0].rtc, host_link) ||
                   swmp_webrtc_link_uses_relay(peers[i].rtc, client_link));
        }
    }

    swmp_lobby_leave(host);
    deadline = now_ms() + 10000;
    for (unsigned i = 1; i < SWMP_MAX_PEERS; i++) {
        while (peers[i].lobby.session.state != SWMP_SESSION_OFFLINE && now_ms() < deadline) tick(peers, SWMP_MAX_PEERS);
        assert(peers[i].lobby.session.state == SWMP_SESSION_OFFLINE);
    }
    for (unsigned i = 0; i < SWMP_MAX_PEERS; i++) destroy(&peers[i]);
    rtcCleanup();
    curl_global_cleanup();
    printf("multiplayer rendezvous: eight %s peers through HTTPS, untrusted TLS rejected, host leave ok\n",
           test_turn ? "TURN relay-only" : "direct");
}
