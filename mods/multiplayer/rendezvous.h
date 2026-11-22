// HTTPS offer/answer client for tools/multiplayer_rendezvous.py.
#pragma once

#include "webrtc.h"

typedef struct SwmpRendezvous SwmpRendezvous;

// The caller owns curl_global_init/cleanup and calls all methods on one owning thread.
// ca_bundle is optional and, if supplied, must remain valid through destroy.
SwmpRendezvous *swmp_rendezvous_create(const char *ca_bundle);
void swmp_rendezvous_destroy(SwmpRendezvous *service);
SwmpWebrtcSignaling swmp_rendezvous_signaling(SwmpRendezvous *service);
void swmp_rendezvous_update(SwmpRendezvous *service, uint64_t now_ms);
