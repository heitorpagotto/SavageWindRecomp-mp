// Exercise the production-facing data-channel adapter with trusted in-process signaling.
#define _POSIX_C_SOURCE 200809L

#include "lobby.h"
#include "webrtc.h"

#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <rtc/rtc.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <time.h>
#endif

#define PROBE_TIMEOUT_MS UINT64_C(10000)

static const char invite[] = "LOCAL-PROBE";

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
    SwmpWebrtc *rtc;
    SwmpLobby lobby;
    unsigned stops;
} Endpoint;

static bool begin(void *context, SwmpWebrtc *rtc, const char *url, const char *code,
                  char *error, size_t error_size) {
    Endpoint *endpoint = context;
    (void)url;
    assert(rtc == endpoint->rtc);
    if (!code) return swmp_webrtc_set_invite(rtc, invite);
    if (!strcmp(code, invite)) return true;
    snprintf(error, error_size, "Unknown local probe invite");
    return false;
}

static void stop(void *context) {
    ((Endpoint *)context)->stops++;
}

static void init(Endpoint *endpoint) {
    endpoint->rtc = swmp_webrtc_create((SwmpWebrtcConfig){.bind_address = "127.0.0.1"},
                                      (SwmpWebrtcSignaling){.context = endpoint, .begin = begin, .stop = stop});
    assert(endpoint->rtc);
    swmp_lobby_init(&endpoint->lobby, swmp_webrtc_transport(endpoint->rtc));
}

static void exchange(Endpoint *from, SwmpConnection connection, char sdp[SWMP_WEBRTC_SDP_SIZE]) {
    uint64_t deadline = now_ms() + PROBE_TIMEOUT_MS;
    int result;
    do {
        result = swmp_webrtc_local_sdp(from->rtc, connection, sdp);
        if (result) break;
        pause_poll();
    } while (now_ms() < deadline);
    assert(result == 1 && strstr(sdp, " 127.0.0.1 "));
}

static void signal_link(Endpoint *host, Endpoint *client, uint8_t nonce,
                        SwmpConnection *host_conn, SwmpConnection *client_conn) {
    // A trusted harness passes complete SDP and a distinct binding directly to both endpoints.
    uint8_t binding[SWMP_BINDING_SIZE] = {nonce};
    assert(!swmp_webrtc_open(client->rtc, (uint8_t[SWMP_BINDING_SIZE]){0}, NULL, now_ms()));
    *client_conn = swmp_webrtc_open(client->rtc, binding, NULL, now_ms());
    assert(*client_conn);
    char sdp[SWMP_WEBRTC_SDP_SIZE];
    exchange(client, *client_conn, sdp);
    *host_conn = swmp_webrtc_open(host->rtc, binding, sdp, now_ms());
    assert(*host_conn && !swmp_webrtc_open(host->rtc, binding, sdp, now_ms()));
    exchange(host, *host_conn, sdp);
    assert(!swmp_webrtc_set_answer(client->rtc, *client_conn, "invalid answer"));
    assert(swmp_webrtc_set_answer(client->rtc, *client_conn, sdp));
    assert(!swmp_webrtc_set_answer(client->rtc, *client_conn, sdp));
}

static void check_channel(SwmpTransport *sender, SwmpConnection send_conn,
                          SwmpTransport *receiver, SwmpConnection recv_conn, SwmpChannel channel) {
    uint8_t payload[] = {'S', 'W', 0, (uint8_t)channel};
    assert(sender->send(sender->context, send_conn, channel, payload, sizeof payload));
    memset(payload, 0, sizeof payload);
    SwmpTransportEvent event;
    uint64_t deadline = now_ms() + PROBE_TIMEOUT_MS;
    while (now_ms() < deadline) {
        if (receiver->poll(receiver->context, &event)) {
            const uint8_t expected[] = {'S', 'W', 0, (uint8_t)channel};
            if (event.type == SWMP_TRANSPORT_PACKET && event.channel == SWMP_CHANNEL_CONTROL)
                continue;  // roster updates can still be in flight
            assert(event.type == SWMP_TRANSPORT_PACKET && event.channel == channel &&
                   event.connection == recv_conn && event.size == sizeof expected &&
                   memcmp(event.data, expected, sizeof expected) == 0);
            return;
        }
        pause_poll();
    }
    assert(!"data channel receive timed out");
}

static void wait_for_join(Endpoint *host, Endpoint *client) {
    uint64_t deadline = now_ms() + PROBE_TIMEOUT_MS;
    while (client->lobby.joining && now_ms() < deadline) {
        uint64_t now = now_ms();
        swmp_lobby_poll(&host->lobby, now);
        swmp_lobby_poll(&client->lobby, now);
        pause_poll();
    }
    assert(!client->lobby.joining);
}

static void wait_for_roster(Endpoint *endpoint, unsigned expected) {
    uint64_t deadline = now_ms() + PROBE_TIMEOUT_MS;
    while (swmp_session_peer_count(&endpoint->lobby.session) != expected && now_ms() < deadline) {
        swmp_lobby_poll(&endpoint->lobby, now_ms());
        pause_poll();
    }
    assert(swmp_session_peer_count(&endpoint->lobby.session) == expected);
}

static void test_lobby(void) {
    Endpoint endpoints[SWMP_MAX_PEERS] = {0}, extra = {0};
    for (unsigned i = 0; i < SWMP_MAX_PEERS; i++) init(&endpoints[i]);
    init(&extra);
    Endpoint *host = &endpoints[0];
    SwmpCompatibility compatibility = {
        .protocol = SWMP_PROTOCOL_VERSION, .mod_version = {0, 1, 0}, .game_version = {1, 9, 0},
        .savage_build = {1}, .game_build = {2},
    };
    assert(swmp_lobby_host(&host->lobby, &compatibility, "Host Link", NULL, 101));
    swmp_lobby_poll(&host->lobby, now_ms());
    assert(!strcmp(host->lobby.invite, invite));

    assert(!swmp_lobby_join(&extra.lobby, &compatibility, "Extra Link", NULL, "invalid", now_ms()));
    assert(extra.stops == 1 && !strcmp(extra.lobby.error, "Unknown local probe invite"));

    SwmpConnection host_connections[SWMP_MAX_PEERS], client_connections[SWMP_MAX_PEERS];
    for (unsigned i = 1; i < SWMP_MAX_PEERS; i++) {
        Endpoint *client = &endpoints[i];
        assert(swmp_lobby_join(&client->lobby, &compatibility, "Client Link", NULL, invite, now_ms()));
        signal_link(host, client, (uint8_t)i, &host_connections[i], &client_connections[i]);
        wait_for_join(host, client);
        assert(client->lobby.session.state == SWMP_SESSION_LOBBY);
        assert(client->lobby.session.session_id == host->lobby.session.session_id);
        assert(client->lobby.session.local_peer == i);
        if (i == 1) {
            check_channel(&host->lobby.transport, host_connections[i], &client->lobby.transport,
                          client_connections[i], SWMP_CHANNEL_STATE);
            check_channel(&client->lobby.transport, client_connections[i], &host->lobby.transport,
                          host_connections[i], SWMP_CHANNEL_EVENT);
        }
    }
    for (unsigned i = 0; i < SWMP_MAX_PEERS; i++) wait_for_roster(&endpoints[i], SWMP_MAX_PEERS);

    SwmpConnection rejected_host, rejected_client;
    assert(swmp_lobby_join(&extra.lobby, &compatibility, "Extra Link", NULL, invite, now_ms()));
    signal_link(host, &extra, 20, &rejected_host, &rejected_client);
    wait_for_join(host, &extra);
    assert(extra.lobby.rejection == SWMP_REJECT_FULL);

    compatibility.game_build[0]++;
    assert(swmp_lobby_join(&extra.lobby, &compatibility, "Extra Link", NULL, invite, now_ms()));
    signal_link(host, &extra, 21, &rejected_host, &rejected_client);
    wait_for_join(host, &extra);
    assert(extra.lobby.rejection == SWMP_REJECT_BUILD);
    assert(swmp_session_peer_count(&host->lobby.session) == SWMP_MAX_PEERS);
    assert(swmp_lobby_start(&host->lobby));
    for (unsigned i = 1; i < SWMP_MAX_PEERS; i++) {
        Endpoint *client = &endpoints[i];
        uint64_t deadline = now_ms() + PROBE_TIMEOUT_MS;
        while (client->lobby.session.state == SWMP_SESSION_LOBBY && now_ms() < deadline) {
            swmp_lobby_poll(&client->lobby, now_ms());
            pause_poll();
        }
        assert(client->lobby.session.state == SWMP_SESSION_PLAYING);
    }

    Endpoint *client = &endpoints[1];
    swmp_lobby_leave(&client->lobby);
    wait_for_roster(host, SWMP_MAX_PEERS - 1);
    for (unsigned i = 2; i < SWMP_MAX_PEERS; i++) wait_for_roster(&endpoints[i], SWMP_MAX_PEERS - 1);

    compatibility.game_build[0]--;
    assert(swmp_lobby_join(&client->lobby, &compatibility, "Rejoined Link", NULL, invite, now_ms()));
    SwmpConnection rejoin_host, rejoin_client;
    signal_link(host, client, 22, &rejoin_host, &rejoin_client);
    assert(rejoin_host > host_connections[1] && rejoin_client > client_connections[1]);
    wait_for_join(host, client);
    assert(client->lobby.session.local_peer == 1 && client->lobby.session.state == SWMP_SESSION_PLAYING);
    for (unsigned i = 0; i < SWMP_MAX_PEERS; i++) wait_for_roster(&endpoints[i], SWMP_MAX_PEERS);

    swmp_lobby_leave(&host->lobby);
    for (unsigned i = 1; i < SWMP_MAX_PEERS; i++) {
        wait_for_roster(&endpoints[i], 0);
        assert(endpoints[i].lobby.session.state == SWMP_SESSION_OFFLINE);
        assert(!strcmp(endpoints[i].lobby.error, "Host disconnected"));
    }

    // The transport can be reused after being stopped, and pending links expire before admission.
    assert(swmp_lobby_join(&client->lobby, &compatibility, "Client Link", NULL, invite, 100));
    assert(swmp_webrtc_open(client->rtc, (uint8_t[SWMP_BINDING_SIZE]){23}, NULL, 100));
    swmp_webrtc_expire(client->rtc, 100 + SWMP_WEBRTC_CONNECT_TIMEOUT_MS);
    swmp_lobby_poll(&client->lobby, 101);
    assert(!client->lobby.joining && !strcmp(client->lobby.error, "Could not establish the WebRTC connection"));

    for (unsigned i = 0; i < SWMP_MAX_PEERS; i++) swmp_webrtc_destroy(endpoints[i].rtc);
    swmp_webrtc_destroy(extra.rtc);
}

static void test_receive_overflow(void) {
    Endpoint host = {0}, client = {0};
    init(&host);
    init(&client);
    const SwmpCompatibility compatibility = {.protocol = SWMP_PROTOCOL_VERSION};
    assert(swmp_lobby_host(&host.lobby, &compatibility, "Host", NULL, 202));
    assert(swmp_lobby_join(&client.lobby, &compatibility, "Client", NULL, invite, now_ms()));
    SwmpConnection host_conn, client_conn;
    signal_link(&host, &client, 1, &host_conn, &client_conn);
    wait_for_join(&host, &client);
    assert(client.lobby.session.state == SWMP_SESSION_LOBBY);

    // Pause receiver polling while the reliable channel callback fills its bounded queue.
    SwmpTransport *transport = &host.lobby.transport;
    const uint8_t byte = 1;
    for (unsigned i = 0; i < 128; i++)
        assert(transport->send(transport->context, host_conn, SWMP_CHANNEL_CONTROL, &byte, sizeof byte));
    uint64_t deadline = now_ms() + 200;
    while (now_ms() < deadline) pause_poll();
    wait_for_roster(&client, 0);
    wait_for_roster(&host, 1);
    swmp_webrtc_destroy(client.rtc);
    swmp_webrtc_destroy(host.rtc);
}

int main(void) {
    rtcPreload();
    test_lobby();
    test_receive_overflow();
    rtcCleanup();
    puts("multiplayer WebRTC loopback: eight peers, rejoin, build/full rejection, channels, overflow, and cleanup ok");
}
