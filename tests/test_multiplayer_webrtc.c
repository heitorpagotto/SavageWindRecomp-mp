// Transport selection probe. Only trusted in-process signaling and loopback sockets.
#define _POSIX_C_SOURCE 200809L

#include "lobby.h"

#undef NDEBUG
#include <assert.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <rtc/rtc.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <time.h>
#endif

#define PACKET_CAPACITY (SWMP_PACKET_HEADER_SIZE + SWMP_MAX_PAYLOAD_SIZE)
#define PROBE_TIMEOUT_MS UINT64_C(10000)

static const char invite[] = "LOCAL-PROBE";
static const char *const labels[] = {"swmp-control", "swmp-state", "swmp-event"};

typedef struct {
    int pc;
    int channels[3];
    atomic_bool gathered;
    bool started;
    bool report_invite;
    bool connected;
    bool disconnected;
    uint8_t packet[PACKET_CAPACITY + 1];  // extra byte for rtcReceiveMessage's string terminator
} ProbeEndpoint;

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

static void RTC_API gathered(int pc, rtcGatheringState state, void *context) {
    (void)pc;
    ProbeEndpoint *endpoint = context;
    if (state == RTC_GATHERING_COMPLETE) atomic_store(&endpoint->gathered, true);
}

static void create(ProbeEndpoint *endpoint) {
    const rtcConfiguration config = {
        .bindAddress = "127.0.0.1",
        .disableAutoNegotiation = true,
        .maxMessageSize = PACKET_CAPACITY,
    };
    atomic_init(&endpoint->gathered, false);
    endpoint->pc = rtcCreatePeerConnection(&config);
    assert(endpoint->pc >= 0);
    rtcSetUserPointer(endpoint->pc, endpoint);
    assert(rtcSetGatheringStateChangeCallback(endpoint->pc, gathered) == RTC_ERR_SUCCESS);
    for (SwmpChannel ch = SWMP_CHANNEL_CONTROL; ch <= SWMP_CHANNEL_EVENT; ch++) {
        bool state = ch == SWMP_CHANNEL_STATE;
        const rtcDataChannelInit init = {
            .reliability = {.unordered = state, .unreliable = state},
            .negotiated = true,
            .manualStream = true,
            .stream = (uint16_t)ch,
        };
        endpoint->channels[ch] = rtcCreateDataChannelEx(endpoint->pc, labels[ch], &init);
        assert(endpoint->channels[ch] >= 0);
        rtcReliability reliability;
        assert(rtcGetDataChannelReliability(endpoint->channels[ch], &reliability) == RTC_ERR_SUCCESS);
        assert(reliability.unordered == state && reliability.unreliable == state);
        if (state) assert(reliability.maxRetransmits == 0 && reliability.maxPacketLifeTime == 0);
    }
}

static void exchange(ProbeEndpoint *from, const ProbeEndpoint *to, const char *type) {
    assert(rtcSetLocalDescription(from->pc, type) == RTC_ERR_SUCCESS);
    uint64_t deadline = now_ms() + PROBE_TIMEOUT_MS;
    while (!atomic_load(&from->gathered) && now_ms() < deadline) pause_poll();
    assert(atomic_load(&from->gathered));

    // The trusted harness passes the original SDP (including its certificate fingerprint and ICE
    // candidates) directly to the other endpoint. Production needs an authenticated equivalent.
    char sdp[8192];
    assert(rtcGetLocalDescription(from->pc, sdp, sizeof sdp) > 0);
    assert(strstr(sdp, "a=fingerprint:") && strstr(sdp, "a=candidate:"));
    assert(rtcSetRemoteDescription(to->pc, sdp, type) == RTC_ERR_SUCCESS);
}

static bool start_host(void *context, const char *url, char *error, size_t error_size) {
    (void)url;
    (void)error;
    (void)error_size;
    ProbeEndpoint *endpoint = context;
    endpoint->started = endpoint->report_invite = true;
    return true;
}

static bool start_join(void *context, const char *url, const char *code, char *error, size_t error_size) {
    (void)url;
    if (!code || strcmp(code, invite) != 0) {
        snprintf(error, error_size, "Unknown local probe invite");
        return false;
    }
    ((ProbeEndpoint *)context)->started = true;
    return true;
}

static bool poll(void *context, SwmpTransportEvent *event) {
    ProbeEndpoint *endpoint = context;
    if (!endpoint->started || endpoint->disconnected) return false;
    *event = (SwmpTransportEvent){.connection = 1};
    if (endpoint->report_invite) {
        endpoint->report_invite = false;
        event->type = SWMP_TRANSPORT_INVITE_READY;
        event->text = invite;
        return true;
    }
    for (SwmpChannel ch = SWMP_CHANNEL_CONTROL; ch <= SWMP_CHANNEL_EVENT; ch++) {
        if (rtcIsClosed(endpoint->channels[ch])) {
            endpoint->disconnected = true;
            event->type = SWMP_TRANSPORT_PEER_DISCONNECTED;
            return true;
        }
        if (!rtcIsOpen(endpoint->channels[ch])) return false;
    }
    if (!endpoint->connected) {
        endpoint->connected = true;
        event->type = SWMP_TRANSPORT_PEER_CONNECTED;
        event->binding[0] = 1;  // single link, bound by the trusted in-process offer/answer exchange
        return true;
    }
    for (SwmpChannel ch = SWMP_CHANNEL_CONTROL; ch <= SWMP_CHANNEL_EVENT; ch++) {
        int size = sizeof endpoint->packet;
        int received = rtcReceiveMessage(endpoint->channels[ch], (char *)endpoint->packet, &size);
        if (received == RTC_ERR_NOT_AVAIL) continue;
        assert(received == RTC_ERR_SUCCESS && size > 0 && (unsigned)size <= PACKET_CAPACITY);
        event->type = SWMP_TRANSPORT_PACKET;
        event->channel = ch;
        event->data = endpoint->packet;
        event->size = (size_t)size;
        return true;
    }
    return false;
}

static bool send_packet(void *context, SwmpConnection connection, SwmpChannel channel,
                        const uint8_t *packet, size_t size) {
    const ProbeEndpoint *endpoint = context;
    if (connection != 1 || !endpoint->connected || endpoint->disconnected ||
        (unsigned)channel > SWMP_CHANNEL_EVENT || size > PACKET_CAPACITY)
        return false;
    return rtcSendMessage(endpoint->channels[channel], (const char *)packet, (int)size) == RTC_ERR_SUCCESS;
}

static void disconnect(void *context, SwmpConnection connection) {
    const ProbeEndpoint *endpoint = context;
    assert(connection == 1);
    assert(rtcClose(endpoint->channels[SWMP_CHANNEL_CONTROL]) == RTC_ERR_SUCCESS);
}

static void stop(void *context) {
    ProbeEndpoint *endpoint = context;
    if (endpoint->started) assert(rtcClosePeerConnection(endpoint->pc) == RTC_ERR_SUCCESS);
    endpoint->started = false;
}

static SwmpTransport transport(ProbeEndpoint *endpoint) {
    return (SwmpTransport){
        .context = endpoint, .start_host = start_host, .start_join = start_join,
        .poll = poll, .send = send_packet, .disconnect = disconnect, .stop = stop,
    };
}

static void destroy(ProbeEndpoint *endpoint) {
    for (SwmpChannel ch = SWMP_CHANNEL_CONTROL; ch <= SWMP_CHANNEL_EVENT; ch++)
        assert(rtcDeleteDataChannel(endpoint->channels[ch]) == RTC_ERR_SUCCESS);
    assert(rtcDeletePeerConnection(endpoint->pc) == RTC_ERR_SUCCESS);
}

static void check_loopback(const ProbeEndpoint *endpoint) {
    char local[512], remote[512];
    assert(rtcGetSelectedCandidatePair(endpoint->pc, local, sizeof local, remote, sizeof remote) > 0);
    assert(strstr(local, " 127.0.0.1 ") && strstr(remote, " 127.0.0.1 "));
}

static void check_data_channel(SwmpTransport *host, SwmpTransport *client, SwmpChannel channel) {
    uint8_t payload[] = {'S', 'W', 0, (uint8_t)channel};
    assert(host->send(host->context, 1, channel, payload, sizeof payload));
    memset(payload, 0, sizeof payload);
    SwmpTransportEvent event;
    uint64_t deadline = now_ms() + PROBE_TIMEOUT_MS;
    while (now_ms() < deadline) {
        if (client->poll(client->context, &event)) {
            const uint8_t expected[] = {'S', 'W', 0, (uint8_t)channel};
            assert(event.type == SWMP_TRANSPORT_PACKET && event.channel == channel &&
                   event.connection == 1 && event.size == sizeof expected &&
                   memcmp(event.data, expected, sizeof expected) == 0);
            return;
        }
        pause_poll();
    }
    assert(!"data channel receive timed out");
}

static void test_lobby(bool incompatible) {
    ProbeEndpoint host_endpoint = {0}, client_endpoint = {0};
    create(&host_endpoint);
    create(&client_endpoint);
    exchange(&client_endpoint, &host_endpoint, "offer");
    exchange(&host_endpoint, &client_endpoint, "answer");

    SwmpCompatibility compatibility = {
        .protocol = SWMP_PROTOCOL_VERSION, .mod_version = {0, 1, 0}, .game_version = {1, 9, 0},
        .savage_build = {1}, .game_build = {2},
    };
    SwmpLobby host, client;
    swmp_lobby_init(&host, transport(&host_endpoint));
    swmp_lobby_init(&client, transport(&client_endpoint));
    assert(swmp_lobby_host(&host, &compatibility, "Host Link", NULL, 101));
    swmp_lobby_poll(&host, now_ms());
    assert(strcmp(host.invite, invite) == 0);
    if (incompatible) compatibility.savage_build[0]++;
    assert(swmp_lobby_join(&client, &compatibility, "Client Link", NULL, host.invite, now_ms()));
    uint64_t deadline = now_ms() + PROBE_TIMEOUT_MS;
    while (client.joining && now_ms() < deadline) {
        swmp_lobby_poll(&host, now_ms());
        swmp_lobby_poll(&client, now_ms());
        pause_poll();
    }
    if (incompatible) {
        assert(client.rejection == SWMP_REJECT_BUILD && client.session.state == SWMP_SESSION_OFFLINE);
        assert(swmp_session_peer_count(&host.session) == 1);
    } else {
        assert(client.session.state == SWMP_SESSION_LOBBY && client.session.session_id == host.session.session_id);
        assert(client.session.local_peer == 1 && swmp_session_peer_count(&client.session) == 2);
        assert(swmp_session_peer_count(&host.session) == 2);
        check_loopback(&host_endpoint);
        check_loopback(&client_endpoint);
        assert(swmp_lobby_start(&host));
        deadline = now_ms() + PROBE_TIMEOUT_MS;
        while (client.session.state == SWMP_SESSION_LOBBY && now_ms() < deadline) {
            swmp_lobby_poll(&client, now_ms());
            pause_poll();
        }
        assert(client.session.state == SWMP_SESSION_PLAYING);
        check_data_channel(&host.transport, &client.transport, SWMP_CHANNEL_STATE);
        check_data_channel(&client.transport, &host.transport, SWMP_CHANNEL_EVENT);
        assert(rtcClose(host_endpoint.channels[SWMP_CHANNEL_CONTROL]) == RTC_ERR_SUCCESS);
        deadline = now_ms() + PROBE_TIMEOUT_MS;
        while (client.session.state != SWMP_SESSION_OFFLINE && now_ms() < deadline) {
            swmp_lobby_poll(&client, now_ms());
            pause_poll();
        }
        assert(client.session.state == SWMP_SESSION_OFFLINE);
        assert(strcmp(client.error, "Host disconnected") == 0);
    }
    swmp_lobby_leave(&client);
    swmp_lobby_leave(&host);
    destroy(&client_endpoint);
    destroy(&host_endpoint);
}

int main(void) {
    rtcPreload();
    test_lobby(false);
    test_lobby(true);
    rtcCleanup();
    puts("multiplayer WebRTC loopback: lobby, build rejection, channels, and disconnect ok");
}
