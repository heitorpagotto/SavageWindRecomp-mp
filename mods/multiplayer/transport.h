// Transport boundary for the multiplayer core. All callbacks run on the lobby/game thread.
#pragma once

#include "lobby_protocol.h"

#define SWMP_INVITE_CODE_SIZE 32u
#define SWMP_TRANSPORT_ERROR_SIZE 160u

// Backend-assigned connection identity. Independent of any peer ID claimed on the wire.
// Nonzero and unique for the lifetime of the transport instance (including across stop/start).
typedef uint32_t SwmpConnection;

typedef enum {
    SWMP_TRANSPORT_INVITE_READY,
    SWMP_TRANSPORT_PEER_CONNECTED,
    SWMP_TRANSPORT_PEER_DISCONNECTED,
    SWMP_TRANSPORT_PACKET,
    SWMP_TRANSPORT_ERROR
} SwmpTransportEventType;

typedef struct {
    SwmpTransportEventType type;
    SwmpConnection connection;
    SwmpChannel channel;
    uint8_t binding[SWMP_BINDING_SIZE];  // same value at both ends of a newly connected link
    const uint8_t *data;
    size_t size;
    const char *text;  // invite code or backend error
} SwmpTransportEvent;

// Production backends must authenticate the connection and derive its binding from authenticated signaling
// or the encrypted channel. Connected is delivered before packets; disconnected invalidates queued packets
// for that connection. Event data/text remain valid until the next poll or stop.
typedef struct {
    void *context;
    bool (*start_host)(void *context, const char *rendezvous_url, char *error, size_t error_size);
    bool (*start_join)(void *context, const char *rendezvous_url, const char *invite_code,
                       char *error, size_t error_size);
    bool (*poll)(void *context, SwmpTransportEvent *event);
    // Copies the packet before returning. False means a reliable packet was not queued.
    bool (*send)(void *context, SwmpConnection connection, SwmpChannel channel,
                 const uint8_t *packet, size_t packet_size);
    // Disconnect an individual link, with a disconnected event at both ends if still running.
    void (*disconnect)(void *context, SwmpConnection connection);
    void (*stop)(void *context);
} SwmpTransport;
