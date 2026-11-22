// Exchange the same invite and reply strings the two menus put on the clipboard.
#define _POSIX_C_SOURCE 200809L
#undef NDEBUG
#include "direct.h"
#include "lobby.h"

#include <assert.h>
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

typedef struct {
    SwmpDirect *direct;
    SwmpWebrtc *rtc;
    SwmpLobby lobby;
} Peer;

static Peer peers[SWMP_MAX_PEERS];
static SwmpCompatibility compatibility = {
    .protocol = SWMP_PROTOCOL_VERSION, .mod_version = {0, 1, 0}, .game_version = {1, 9, 0},
    .savage_build = {1}, .game_build = {2},
};

static uint64_t now_ms(void) {
#ifdef _WIN32
    return GetTickCount64();
#else
    struct timespec now;
    assert(clock_gettime(CLOCK_MONOTONIC, &now) == 0);
    return (uint64_t)now.tv_sec * 1000 + now.tv_nsec / 1000000;
#endif
}

static void tick(void) {
    uint64_t now = now_ms();
    for (unsigned i = 0; i < SWMP_MAX_PEERS; i++) {
        Peer *p = &peers[i];
        swmp_direct_update(p->direct, now);
        swmp_webrtc_expire(p->rtc, now);
        swmp_lobby_poll(&p->lobby, now);
    }
#ifdef _WIN32
    Sleep(1);
#else
    nanosleep(&(struct timespec){.tv_nsec = 1000000}, NULL);
#endif
}

static const char *message(Peer *p, SwmpDirectState want) {
    uint64_t end = now_ms() + 12000;
    while (swmp_direct_state(p->direct) == SWMP_DIRECT_PREPARING && now_ms() < end) tick();
    assert(swmp_direct_state(p->direct) == want);
    return swmp_direct_message(p->direct);
}

static const char *reply_to(Peer *p, const char *invite) {
    assert(swmp_lobby_join(&p->lobby, &compatibility, "Client", NULL, invite, now_ms()));
    return message(p, SWMP_DIRECT_REPLY);
}

int main(void) {
    rtcPreload();
    const char *stun = getenv("SWMP_TEST_STUN");
    SwmpWebrtcConfig config = {
        .bind_address = stun ? NULL : "127.0.0.1", .ice_servers = &stun, .ice_server_count = stun ? 1 : 0,
        .connect_timeout_ms = SWMP_DIRECT_TIMEOUT_MS,
    };
    for (unsigned i = 0; i < SWMP_MAX_PEERS; i++) {
        Peer *p = &peers[i];
        p->direct = swmp_direct_create();
        assert(p->direct);
        p->rtc = swmp_webrtc_create(config, swmp_direct_signaling(p->direct));
        assert(p->rtc);
        swmp_lobby_init(&p->lobby, swmp_webrtc_transport(p->rtc));
    }

    Peer *host = &peers[0];
    assert(swmp_lobby_host(&host->lobby, &compatibility, "Host", NULL, 42));
    assert(!swmp_lobby_join(&peers[1].lobby, &compatibility, "Client", NULL, "SWMP1I.invalid", now_ms()));
    const char *invite = message(host, SWMP_DIRECT_INVITE);
    const char *stale = reply_to(&peers[1], invite);
    swmp_direct_new_invite(host->direct);
    message(host, SWMP_DIRECT_INVITE);
    assert(!swmp_direct_accept_reply(host->direct, stale));
    swmp_lobby_leave(&peers[1].lobby);

    for (unsigned i = 1; i < SWMP_MAX_PEERS; i++) {
        if (i > 1) swmp_direct_new_invite(host->direct);
        invite = message(host, SWMP_DIRECT_INVITE);
        assert(!swmp_direct_accept_reply(host->direct, invite));
        const char *reply = reply_to(&peers[i], invite);
        assert(swmp_direct_accept_reply(host->direct, reply));
        assert(!swmp_direct_accept_reply(host->direct, reply));
        uint64_t end = now_ms() + 12000;
        while (swmp_session_peer_count(&peers[i].lobby.session) != i + 1 && now_ms() < end) tick();
        assert(swmp_session_peer_count(&peers[i].lobby.session) == i + 1);
    }
    uint64_t end = now_ms() + 12000;
    while (swmp_session_peer_count(&peers[1].lobby.session) != SWMP_MAX_PEERS && now_ms() < end) tick();
    assert(swmp_session_peer_count(&peers[1].lobby.session) == SWMP_MAX_PEERS);
    assert(swmp_lobby_start(&host->lobby));
    end = now_ms() + 12000;
    while (peers[7].lobby.session.state != SWMP_SESSION_PLAYING && now_ms() < end) tick();
    assert(peers[7].lobby.session.state == SWMP_SESSION_PLAYING);
    swmp_direct_new_invite(host->direct);
    message(host, SWMP_DIRECT_INVITE);
    swmp_direct_update(host->direct, now_ms() + SWMP_DIRECT_TIMEOUT_MS);
    assert(swmp_direct_state(host->direct) == SWMP_DIRECT_EXPIRED && !swmp_direct_message(host->direct));
    assert(swmp_session_peer_count(&host->lobby.session) == SWMP_MAX_PEERS);

    swmp_lobby_leave(&host->lobby);
    end = now_ms() + 12000;
    while (peers[7].lobby.session.state != SWMP_SESSION_OFFLINE && now_ms() < end) tick();
    assert(peers[7].lobby.session.state == SWMP_SESSION_OFFLINE);
    for (unsigned i = 0; i < SWMP_MAX_PEERS; i++) {
        swmp_webrtc_destroy(peers[i].rtc);
        swmp_direct_destroy(peers[i].direct);
    }
    rtcCleanup();
    puts("multiplayer direct: eight peers, stale and invalid messages, single-use replies, expiry and cleanup ok");
}
