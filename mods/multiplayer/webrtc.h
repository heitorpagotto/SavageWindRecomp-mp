// WebRTC transport and the offer/answer signaling boundary.
#pragma once

#include "transport.h"

#define SWMP_WEBRTC_SDP_SIZE 16384u
#define SWMP_WEBRTC_CONNECT_TIMEOUT_MS UINT64_C(45000)

typedef struct SwmpWebrtc SwmpWebrtc;

typedef struct {
    const char **ice_servers;
    int ice_server_count;
    const char *bind_address;
    bool relay_only;
    uint64_t connect_timeout_ms;  // zero uses the default
} SwmpWebrtcConfig;

typedef struct {
    void *context;
    // Begin a signaling session. A NULL invite means host. Roll back via stop on failure.
    bool (*begin)(void *context, SwmpWebrtc *rtc, const char *url, const char *invite,
                  char *error, size_t error_size);
    void (*stop)(void *context);
} SwmpWebrtcSignaling;

// The owner thread calls all functions. Config strings live through destroy. The process owns
// the libdatachannel preload/cleanup lifecycle and cleans up after all endpoints are destroyed.
SwmpWebrtc *swmp_webrtc_create(SwmpWebrtcConfig config, SwmpWebrtcSignaling signaling);
void swmp_webrtc_destroy(SwmpWebrtc *rtc);
SwmpTransport swmp_webrtc_transport(SwmpWebrtc *rtc);

// The signaling provider exchanges SDP over a trusted channel (including certificate fingerprints)
// and supplies a fresh, nonzero binding. Either lobby role can make the offer.
// A NULL offer starts an offer. A zero handle indicates failure.
SwmpConnection swmp_webrtc_open(SwmpWebrtc *rtc, const uint8_t binding[SWMP_BINDING_SIZE],
                               const char *offer, uint64_t now_ms);
bool swmp_webrtc_set_answer(SwmpWebrtc *rtc, SwmpConnection connection, const char *answer);
// Returns 1 for complete SDP, 0 while gathering, and -1 on failure.
int swmp_webrtc_local_sdp(SwmpWebrtc *rtc, SwmpConnection connection, char out[SWMP_WEBRTC_SDP_SIZE]);
bool swmp_webrtc_set_invite(SwmpWebrtc *rtc, const char *invite);
void swmp_webrtc_signal_error(SwmpWebrtc *rtc, const char *error);
void swmp_webrtc_expire(SwmpWebrtc *rtc, uint64_t now_ms);
bool swmp_webrtc_has_link(SwmpWebrtc *rtc, SwmpConnection connection);
bool swmp_webrtc_link_uses_relay(SwmpWebrtc *rtc, SwmpConnection connection);
