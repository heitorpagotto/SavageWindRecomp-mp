// Transport boundary for the multiplayer core. A backend may use WebRTC or another encrypted ICE-capable transport.
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "protocol.h"

#define SWMP_INVITE_CODE_SIZE 32u
#define SWMP_TRANSPORT_ERROR_SIZE 160u

typedef enum {
    SWMP_TRANSPORT_INVITE_READY,
    SWMP_TRANSPORT_PEER_CONNECTED,
    SWMP_TRANSPORT_PEER_DISCONNECTED,
    SWMP_TRANSPORT_PACKET,
    SWMP_TRANSPORT_ERROR
} SwmpTransportEventType;

typedef struct {
    SwmpTransportEventType type;
    uint8_t peer_id;
    SwmpChannel channel;
    const uint8_t *data;  // owned by the backend, valid until the next poll
    size_t size;
    const char *text;     // invite code, disconnect reason, or error
} SwmpTransportEvent;

typedef struct {
    void *context;
    bool (*start_host)(void *context, const char *rendezvous_url, char *error, size_t error_size);
    bool (*start_join)(void *context, const char *rendezvous_url, const char *invite_code,
                       char *error, size_t error_size);
    bool (*poll)(void *context, SwmpTransportEvent *event);
    bool (*send)(void *context, uint8_t peer_id, SwmpChannel channel,
                 const uint8_t *packet, size_t packet_size);
    void (*stop)(void *context);
} SwmpTransport;

