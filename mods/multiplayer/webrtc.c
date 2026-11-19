#include "webrtc.h"

#include <rtc/rtc.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RTC_LINKS (2u * SWMP_MAX_PEERS)
#define PACKET_CAPACITY (SWMP_PACKET_HEADER_SIZE + SWMP_MAX_PAYLOAD_SIZE)
#define BUFFER_LIMIT (4 * (int)PACKET_CAPACITY)
#define RECEIVE_SLOTS 17u

typedef struct {
    size_t size;
    uint8_t data[];
} ReceivedPacket;

typedef struct {
    int id;
    bool replaceable;
    atomic_bool failed;
    _Atomic(ReceivedPacket *) latest;
    atomic_uint read_pos, write_pos;
    ReceivedPacket *queued[RECEIVE_SLOTS];
} SwmpWebrtcChannel;

typedef struct {
    SwmpConnection connection;
    uint64_t opened_at;
    uint8_t binding[SWMP_BINDING_SIZE];
    int pc;
    SwmpWebrtcChannel channels[SWMP_CHANNEL_COUNT];
    bool remote_sdp;
    bool connected;
    bool closing;
} SwmpWebrtcLink;

struct SwmpWebrtc {
    SwmpWebrtcConfig config;
    SwmpWebrtcSignaling signaling;
    SwmpWebrtcLink links[RTC_LINKS];
    SwmpConnection last_connection;
    unsigned next_link;
    bool active;
    bool hosting;
    bool invite_pending;
    char invite[SWMP_INVITE_CODE_SIZE];
    char error[SWMP_TRANSPORT_ERROR_SIZE];
    ReceivedPacket *event_packet;
};

static SwmpWebrtcLink *find_link(SwmpWebrtc *rtc, SwmpConnection connection) {
    if (connection)
        for (unsigned i = 0; i < RTC_LINKS; i++)
            if (rtc->links[i].connection == connection) return &rtc->links[i];
    return NULL;
}

// libdatachannel serializes message callbacks per channel. The owner thread is the sole reader.
static void RTC_API receive_message(int id, const char *data, int size, void *context) {
    (void)id;
    SwmpWebrtcChannel *ch = context;
    if (atomic_load(&ch->failed)) return;
    if (size <= 0 || (unsigned)size > PACKET_CAPACITY) {
        atomic_store(&ch->failed, true);
        return;
    }

    unsigned pos = atomic_load(&ch->write_pos);
    unsigned next = (pos + 1) % RECEIVE_SLOTS;
    if (!ch->replaceable && next == atomic_load(&ch->read_pos)) {
        atomic_store(&ch->failed, true);
        return;
    }

    ReceivedPacket *packet = malloc(sizeof *packet + (size_t)size);
    if (!packet) {
        if (!ch->replaceable) atomic_store(&ch->failed, true);
        return;
    }
    packet->size = (size_t)size;
    memcpy(packet->data, data, packet->size);
    if (ch->replaceable) {
        free(atomic_exchange(&ch->latest, packet));
    } else {
        ch->queued[pos] = packet;
        atomic_store(&ch->write_pos, next);
    }
}

static ReceivedPacket *pop_packet(SwmpWebrtcChannel *ch) {
    if (ch->replaceable) return atomic_exchange(&ch->latest, NULL);
    unsigned pos = atomic_load(&ch->read_pos);
    if (pos == atomic_load(&ch->write_pos)) return NULL;
    ReceivedPacket *packet = ch->queued[pos];
    atomic_store(&ch->read_pos, (pos + 1) % RECEIVE_SLOTS);
    return packet;
}

static void release_link(SwmpWebrtcLink *link) {
    for (unsigned i = 0; i < SWMP_CHANNEL_COUNT; i++) {
        SwmpWebrtcChannel *ch = &link->channels[i];
        if (ch->id < 0) continue;
        rtcSetMessageCallback(ch->id, NULL);  // waits for the active callback before reclaiming its queue
        rtcSetUserPointer(ch->id, NULL);
        rtcDeleteDataChannel(ch->id);
        ReceivedPacket *packet;
        while ((packet = pop_packet(ch))) free(packet);
    }
    if (link->pc >= 0) rtcDeletePeerConnection(link->pc);
    *link = (SwmpWebrtcLink){0};
}

static void stop(void *context) {
    SwmpWebrtc *rtc = context;
    if (!rtc->active) return;
    rtc->active = false;
    rtc->signaling.stop(rtc->signaling.context);
    for (unsigned i = 0; i < RTC_LINKS; i++)
        if (rtc->links[i].connection) release_link(&rtc->links[i]);
    free(rtc->event_packet);
    rtc->event_packet = NULL;
    rtc->invite_pending = false;
    rtc->invite[0] = rtc->error[0] = 0;
    rtc->next_link = 0;
}

SwmpWebrtc *swmp_webrtc_create(SwmpWebrtcConfig config, SwmpWebrtcSignaling signaling) {
    if (!signaling.begin || !signaling.stop) return NULL;
    SwmpWebrtc *rtc = calloc(1, sizeof *rtc);
    if (rtc) {
        rtc->config = config;
        rtc->signaling = signaling;
    }
    return rtc;
}

void swmp_webrtc_destroy(SwmpWebrtc *rtc) {
    if (rtc) stop(rtc);
    free(rtc);
}

static bool begin(SwmpWebrtc *rtc, const char *url, const char *invite, char *error, size_t error_size) {
    if (rtc->active) return false;
    rtc->active = true;
    rtc->hosting = !invite;
    if (rtc->signaling.begin(rtc->signaling.context, rtc, url, invite, error, error_size)) return true;
    stop(rtc);
    return false;
}

static bool start_host(void *context, const char *url, char *error, size_t error_size) {
    return begin(context, url, NULL, error, error_size);
}

static bool start_join(void *context, const char *url, const char *invite, char *error, size_t error_size) {
    if (!invite || !*invite || strlen(invite) >= SWMP_INVITE_CODE_SIZE) {
        snprintf(error, error_size, "Invalid lobby invite");
        return false;
    }
    return begin(context, url, invite, error, error_size);
}

static bool valid_sdp(const char *sdp) {
    return sdp && strlen(sdp) < SWMP_WEBRTC_SDP_SIZE && strstr(sdp, "a=fingerprint:") &&
           strstr(sdp, "a=candidate:") && strstr(sdp, "a=end-of-candidates");
}

SwmpConnection swmp_webrtc_open(SwmpWebrtc *rtc, const uint8_t binding[SWMP_BINDING_SIZE],
                               const char *offer, uint64_t now_ms) {
    static const uint8_t zero[SWMP_BINDING_SIZE];
    if (!rtc->active || !binding || !memcmp(binding, zero, sizeof zero) ||
        (rtc->hosting ? !valid_sdp(offer) : offer != NULL) || rtc->last_connection == UINT32_MAX)
        return 0;
    SwmpWebrtcLink *link = NULL;
    for (unsigned i = 0; i < RTC_LINKS; i++) {
        SwmpWebrtcLink *other = &rtc->links[i];
        if (!other->connection) {
            if (!link) link = other;
        } else if (!rtc->hosting || !memcmp(other->binding, binding, sizeof zero)) {
            return 0;
        }
    }
    if (!link) return 0;

    const rtcConfiguration config = {
        .iceServers = rtc->config.ice_servers,
        .iceServersCount = rtc->config.ice_server_count,
        .bindAddress = rtc->config.bind_address,
        .iceTransportPolicy = rtc->config.relay_only ? RTC_TRANSPORT_POLICY_RELAY : RTC_TRANSPORT_POLICY_ALL,
        .disableAutoNegotiation = true,
        .maxMessageSize = PACKET_CAPACITY,
    };
    *link = (SwmpWebrtcLink){.pc = rtcCreatePeerConnection(&config), .opened_at = now_ms};
    for (unsigned i = 0; i < SWMP_CHANNEL_COUNT; i++) link->channels[i].id = -1;
    if (link->pc < 0) goto failed;
    static const char *const labels[] = {"swmp-control", "swmp-state", "swmp-event"};
    for (unsigned i = 0; i < SWMP_CHANNEL_COUNT; i++) {
        SwmpWebrtcChannel *ch = &link->channels[i];
        bool state = ch->replaceable = i == SWMP_CHANNEL_STATE;
        atomic_init(&ch->failed, false);
        atomic_init(&ch->latest, NULL);
        atomic_init(&ch->read_pos, 0);
        atomic_init(&ch->write_pos, 0);
        const rtcDataChannelInit init = {
            .reliability = {.unordered = state, .unreliable = state},
            .negotiated = true,
            .manualStream = true,
            .stream = (uint16_t)i,
        };
        ch->id = rtcCreateDataChannelEx(link->pc, labels[i], &init);
        if (ch->id < 0) goto failed;
        rtcSetUserPointer(ch->id, ch);
        if (rtcSetMessageCallback(ch->id, receive_message) != RTC_ERR_SUCCESS) goto failed;
    }
    if (offer && rtcSetRemoteDescription(link->pc, offer, "offer") != RTC_ERR_SUCCESS) goto failed;
    if (rtcSetLocalDescription(link->pc, offer ? "answer" : "offer") != RTC_ERR_SUCCESS) goto failed;

    link->remote_sdp = offer != NULL;
    memcpy(link->binding, binding, sizeof link->binding);
    return link->connection = ++rtc->last_connection;
failed:
    release_link(link);
    return 0;
}

bool swmp_webrtc_set_answer(SwmpWebrtc *rtc, SwmpConnection connection, const char *answer) {
    SwmpWebrtcLink *link = find_link(rtc, connection);
    if (!link || link->closing || rtc->hosting || link->remote_sdp || !valid_sdp(answer)) return false;
    link->remote_sdp = rtcSetRemoteDescription(link->pc, answer, "answer") == RTC_ERR_SUCCESS;
    if (!link->remote_sdp) link->closing = true;
    return link->remote_sdp;
}

int swmp_webrtc_local_sdp(SwmpWebrtc *rtc, SwmpConnection connection, char out[SWMP_WEBRTC_SDP_SIZE]) {
    SwmpWebrtcLink *link = find_link(rtc, connection);
    if (!link || link->closing) return -1;
    int size = rtcGetLocalDescription(link->pc, out, SWMP_WEBRTC_SDP_SIZE);
    if (size == RTC_ERR_NOT_AVAIL) return 0;
    if (size <= 0) return -1;
    if (!strstr(out, "a=end-of-candidates")) return 0;
    return valid_sdp(out) ? 1 : -1;
}

bool swmp_webrtc_set_invite(SwmpWebrtc *rtc, const char *invite) {
    if (!rtc->active || !rtc->hosting || !invite || !*invite || strlen(invite) >= sizeof rtc->invite)
        return false;
    strcpy(rtc->invite, invite);
    rtc->invite_pending = true;
    return true;
}

void swmp_webrtc_signal_error(SwmpWebrtc *rtc, const char *error) {
    if (rtc->active) snprintf(rtc->error, sizeof rtc->error, "%s", error);
}

void swmp_webrtc_expire(SwmpWebrtc *rtc, uint64_t now_ms) {
    for (unsigned i = 0; i < RTC_LINKS; i++) {
        SwmpWebrtcLink *link = &rtc->links[i];
        if (link->connection && !link->connected && now_ms - link->opened_at >= SWMP_WEBRTC_CONNECT_TIMEOUT_MS)
            link->closing = true;
    }
}

static bool poll_link(SwmpWebrtc *rtc, SwmpWebrtcLink *link, SwmpTransportEvent *event) {
    if (!link->connection) return false;
    *event = (SwmpTransportEvent){.connection = link->connection};
    if (link->closing) goto disconnected;

    bool all_open = true, any_closed = false;
    for (unsigned i = 0; i < SWMP_CHANNEL_COUNT; i++) {
        SwmpWebrtcChannel *ch = &link->channels[i];
        if (atomic_load(&ch->failed)) goto disconnected;
        if (rtcIsClosed(ch->id)) any_closed = true;
        if (!rtcIsOpen(ch->id)) all_open = false;
    }
    if (!link->connected) {
        if (any_closed) goto disconnected;
        if (!link->remote_sdp || !all_open) return false;
        link->connected = true;
        event->type = SWMP_TRANSPORT_PEER_CONNECTED;
        memcpy(event->binding, link->binding, sizeof event->binding);
        return true;
    }

    // Consume a queued control response before reporting a remote channel close.
    static const SwmpChannel order[] = {SWMP_CHANNEL_CONTROL, SWMP_CHANNEL_EVENT, SWMP_CHANNEL_STATE};
    for (unsigned i = 0; i < 3; i++) {
        SwmpChannel ch = order[i];
        if (any_closed && ch != SWMP_CHANNEL_CONTROL) break;
        ReceivedPacket *packet = pop_packet(&link->channels[ch]);
        if (!packet) continue;
        rtc->event_packet = packet;
        event->type = SWMP_TRANSPORT_PACKET;
        event->channel = ch;
        event->data = packet->data;
        event->size = packet->size;
        return true;
    }
    if (!any_closed) return false;
disconnected:
    if (!rtc->hosting && !link->connected) {
        event->type = SWMP_TRANSPORT_ERROR;
        event->text = "Could not establish the WebRTC connection";
    } else {
        event->type = SWMP_TRANSPORT_PEER_DISCONNECTED;
    }
    release_link(link);
    return true;
}

static bool poll(void *context, SwmpTransportEvent *event) {
    SwmpWebrtc *rtc = context;
    free(rtc->event_packet);
    rtc->event_packet = NULL;
    if (!rtc->active) return false;
    if (rtc->error[0]) {
        *event = (SwmpTransportEvent){.type = SWMP_TRANSPORT_ERROR, .text = rtc->error};
        return true;  // terminal; the owner stops the transport
    }
    if (rtc->invite_pending) {
        rtc->invite_pending = false;
        *event = (SwmpTransportEvent){.type = SWMP_TRANSPORT_INVITE_READY, .text = rtc->invite};
        return true;
    }
    for (unsigned i = 0; i < RTC_LINKS; i++) {
        unsigned pos = (rtc->next_link + i) % RTC_LINKS;
        if (poll_link(rtc, &rtc->links[pos], event)) {
            rtc->next_link = (pos + 1) % RTC_LINKS;
            return true;
        }
    }
    return false;
}

bool swmp_webrtc_link_uses_relay(SwmpWebrtc *rtc, SwmpConnection connection) {
    SwmpWebrtcLink *link = find_link(rtc, connection);
    char local[512], remote[512];
    return link && link->connected &&
           rtcGetSelectedCandidatePair(link->pc, local, sizeof local, remote, sizeof remote) > 0 &&
           (strstr(local, " typ relay") || strstr(remote, " typ relay"));
}

static bool send_packet(void *context, SwmpConnection connection, SwmpChannel ch,
                        const uint8_t *packet, size_t size) {
    SwmpWebrtcLink *link = find_link(context, connection);
    if (!link || !link->connected || link->closing || (unsigned)ch > SWMP_CHANNEL_EVENT ||
        !packet || !size || size > PACKET_CAPACITY) return false;
    int channel = link->channels[ch].id;
    if (!rtcIsOpen(channel)) return false;
    int buffered = rtcGetBufferedAmount(channel);
    int limit = ch == SWMP_CHANNEL_STATE ? (int)PACKET_CAPACITY : BUFFER_LIMIT;
    return buffered >= 0 && buffered <= limit - (int)size &&
           rtcSendMessage(channel, (const char *)packet, (int)size) == RTC_ERR_SUCCESS;
}

static void disconnect(void *context, SwmpConnection connection) {
    SwmpWebrtcLink *link = find_link(context, connection);
    if (link) link->closing = true;
}

SwmpTransport swmp_webrtc_transport(SwmpWebrtc *rtc) {
    return (SwmpTransport){
        .context = rtc, .start_host = start_host, .start_join = start_join, .poll = poll,
        .send = send_packet, .disconnect = disconnect, .stop = stop,
        .connect_timeout_ms = SWMP_WEBRTC_CONNECT_TIMEOUT_MS,
    };
}
