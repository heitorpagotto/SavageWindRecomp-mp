// Clipboard signaling for a host offer and a single-use reply.
#pragma once
#include "webrtc.h"

#define SWMP_DIRECT_TIMEOUT_MS UINT64_C(300000)

typedef struct SwmpDirect SwmpDirect;

typedef enum {
    SWMP_DIRECT_IDLE,
    SWMP_DIRECT_PREPARING,
    SWMP_DIRECT_INVITE,
    SWMP_DIRECT_REPLY,
    SWMP_DIRECT_EXPIRED,
} SwmpDirectState;

SwmpDirect *swmp_direct_create(void);
void swmp_direct_destroy(SwmpDirect *direct);
SwmpWebrtcSignaling swmp_direct_signaling(SwmpDirect *direct);
void swmp_direct_update(SwmpDirect *direct, uint64_t now);
void swmp_direct_new_invite(SwmpDirect *direct);
SwmpConnection swmp_direct_accept_reply(SwmpDirect *direct, const char *reply);
SwmpDirectState swmp_direct_state(const SwmpDirect *direct);
const char *swmp_direct_message(const SwmpDirect *direct);
