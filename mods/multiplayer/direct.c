#include "direct.h"
#include "random.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PAYLOAD_SIZE (SWMP_BINDING_SIZE + SWMP_WEBRTC_SDP_SIZE - 1)
#define MESSAGE_SIZE (sizeof "SWMP1I." + (4 * PAYLOAD_SIZE + 2) / 3)

struct SwmpDirect {
    SwmpWebrtc *rtc;
    SwmpConnection link;
    uint64_t opened_at;
    bool host;
    SwmpDirectState state;
    uint8_t payload[PAYLOAD_SIZE + 1];
    char message[MESSAGE_SIZE];
};

static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

// A single line survives clipboard and chat line-ending conversion. The payload starts with the
// per-invite binding, followed by the gathered SDP (including the DTLS fingerprint).
static void encode(SwmpDirect *d) {
    char *out = d->message;
    out += sprintf(out, "SWMP1%c.", d->host ? 'I' : 'R');
    size_t length = SWMP_BINDING_SIZE + strlen((char *)d->payload + SWMP_BINDING_SIZE);
    uint32_t bits = 0;
    unsigned count = 0;
    for (size_t i = 0; i < length; i++) {
        bits = (bits << 8) | d->payload[i];
        count += 8;
        while (count >= 6) {
            count -= 6;
            *out++ = alphabet[(bits >> count) & 63];
        }
    }
    if (count) *out++ = alphabet[(bits << (6 - count)) & 63];
    *out = '\0';
}

static bool decode(const char *text, const char *prefix, uint8_t payload[PAYLOAD_SIZE + 1]) {
    if (!text) return false;
    text += strspn(text, " \t\r\n");
    size_t prefix_length = strlen(prefix);
    if (strncmp(text, prefix, prefix_length)) return false;
    text += prefix_length;
    size_t length = 0;
    unsigned count = 0;
    uint32_t bits = 0;
    while (*text && !strchr(" \t\r\n", *text)) {
        const char *digit = strchr(alphabet, *text++);
        if (!digit) return false;
        bits = (bits << 6) | (uint32_t)(digit - alphabet);
        count += 6;
        if (count >= 8) {
            count -= 8;
            if (length == PAYLOAD_SIZE) return false;
            payload[length++] = (uint8_t)(bits >> count);
        }
    }
    if (count >= 6 || (bits & ((1u << count) - 1)) || text[strspn(text, " \t\r\n")]) return false;
    if (length < SWMP_BINDING_SIZE + 5 ||
        memchr(payload + SWMP_BINDING_SIZE, 0, length - SWMP_BINDING_SIZE) ||
        memcmp(payload + SWMP_BINDING_SIZE, "v=0\r\n", 5)) return false;
    payload[length] = '\0';
    return true;
}

static void stop(void *context) {
    SwmpDirect *d = context;
    d->rtc = NULL;
    d->link = 0;
    d->state = SWMP_DIRECT_IDLE;
    d->message[0] = 0;
}

static bool begin(void *context, SwmpWebrtc *rtc, const char *url, const char *invite,
                  char *error, size_t error_size) {
    (void)url;
    SwmpDirect *d = context;
    if (invite && !decode(invite, "SWMP1I.", d->payload)) {
        snprintf(error, error_size, "Invalid direct multiplayer invite");
        return false;
    }
    d->rtc = rtc;
    d->host = invite == NULL;
    d->state = SWMP_DIRECT_PREPARING;
    return true;
}

SwmpDirect *swmp_direct_create(void) { return calloc(1, sizeof(SwmpDirect)); }
void swmp_direct_destroy(SwmpDirect *d) { free(d); }

SwmpWebrtcSignaling swmp_direct_signaling(SwmpDirect *d) {
    return (SwmpWebrtcSignaling){.context = d, .begin = begin, .stop = stop};
}

SwmpDirectState swmp_direct_state(const SwmpDirect *d) {
    return d ? d->state : SWMP_DIRECT_IDLE;
}

const char *swmp_direct_message(const SwmpDirect *d) {
    return d && (d->state == SWMP_DIRECT_INVITE || d->state == SWMP_DIRECT_REPLY) ? d->message : NULL;
}

static void discard_link(SwmpDirect *d) {
    if (d->link) {
        SwmpTransport t = swmp_webrtc_transport(d->rtc);
        t.disconnect(t.context, d->link);
    }
    d->link = 0;
    d->message[0] = 0;
}

void swmp_direct_new_invite(SwmpDirect *d) {
    if (!d || !d->rtc || !d->host) return;
    discard_link(d);
    d->state = SWMP_DIRECT_PREPARING;
}

SwmpConnection swmp_direct_accept_reply(SwmpDirect *d, const char *reply) {
    uint8_t payload[PAYLOAD_SIZE + 1];
    if (!d || d->state != SWMP_DIRECT_INVITE || !decode(reply, "SWMP1R.", payload) ||
        memcmp(payload, d->payload, SWMP_BINDING_SIZE)) return 0;
    if (!swmp_webrtc_set_answer(d->rtc, d->link, (char *)payload + SWMP_BINDING_SIZE)) {
        discard_link(d);
        d->state = SWMP_DIRECT_EXPIRED;
        return 0;
    }
    // The transport now owns this peer. The next invitation gets a new binding and connection.
    SwmpConnection connection = d->link;
    d->link = 0;
    d->message[0] = 0;
    d->state = SWMP_DIRECT_IDLE;
    return connection;
}

void swmp_direct_update(SwmpDirect *d, uint64_t now) {
    if (d->host && d->link && now - d->opened_at >= SWMP_DIRECT_TIMEOUT_MS) {
        discard_link(d);
        d->state = SWMP_DIRECT_EXPIRED;
        return;
    }
    if (d->state != SWMP_DIRECT_PREPARING) return;
    int result = -1;
    if (!d->link && (!d->host || swmp_random_bytes(d->payload, SWMP_BINDING_SIZE))) {
        d->link = swmp_webrtc_open(d->rtc, d->payload, d->host ? NULL : (char *)d->payload + SWMP_BINDING_SIZE, now);
        d->opened_at = now;
    }
    if (d->link) result = swmp_webrtc_local_sdp(d->rtc, d->link, (char *)d->payload + SWMP_BINDING_SIZE);
    if (result < 0) {
        discard_link(d);
        d->state = SWMP_DIRECT_EXPIRED;
        if (!d->host) swmp_webrtc_signal_error(d->rtc, "Could not prepare the direct multiplayer reply");
    } else if (result > 0) {
        encode(d);
        d->state = d->host ? SWMP_DIRECT_INVITE : SWMP_DIRECT_REPLY;
    }
}
