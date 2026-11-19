#include "mock_transport.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    SwmpConnection connection;
    SwmpChannel channel;
    size_t size;
    uint8_t *data;
} MockPacket;

typedef struct {
    SwmpMock *mock;
    unsigned index;
    bool running;
    bool invite_pending;
    SwmpMockFault fault;
    MockPacket queue[SWMP_MOCK_QUEUE_SIZE];
    size_t count;
    uint8_t *polled_data;
    char polled_text[SWMP_INVITE_CODE_SIZE];
} MockEndpoint;

typedef struct {
    SwmpConnection id;
    unsigned endpoints[2];  // host, client
    bool open;
    bool pending[2];       // connected when open, disconnected when closed
} MockLink;

struct SwmpMock {
    MockEndpoint endpoints[SWMP_MOCK_ENDPOINTS];
    MockLink links[SWMP_MOCK_ENDPOINTS - 1];
    SwmpConnection last_connection;
    unsigned host;         // SWMP_MOCK_ENDPOINTS when offline
    unsigned generation;
    char invite[SWMP_INVITE_CODE_SIZE];
};

static int link_side(const MockLink *link, const MockEndpoint *ep) {
    if (link->id && link->endpoints[0] == ep->index) return 0;
    if (link->id && link->endpoints[1] == ep->index) return 1;
    return -1;
}

static MockLink *find_link(MockEndpoint *ep, SwmpConnection id) {
    for (size_t i = 0; i < SWMP_MOCK_ENDPOINTS - 1; i++) {
        MockLink *link = &ep->mock->links[i];
        if (link->open && link->id == id && link_side(link, ep) >= 0) return link;
    }
    return NULL;
}

static MockPacket remove_packet(MockEndpoint *ep, size_t index) {
    MockPacket packet = ep->queue[index];
    ep->count--;
    memmove(&ep->queue[index], &ep->queue[index + 1], (ep->count - index) * sizeof packet);
    return packet;
}

static void reap_link(MockLink *link) {
    if (!link->open && !link->pending[0] && !link->pending[1]) *link = (MockLink){0};
}

static void close_link(SwmpMock *mock, MockLink *link) {
    link->open = false;
    for (int side = 0; side < 2; side++) {
        MockEndpoint *ep = &mock->endpoints[link->endpoints[side]];
        link->pending[side] = ep->running;
        for (size_t i = ep->count; i > 0; i--)
            if (ep->queue[i - 1].connection == link->id) free(remove_packet(ep, i - 1).data);
    }
    reap_link(link);
}

static bool start_error(char *error, size_t size, const char *message) {
    if (error && size) snprintf(error, size, "%s", message);
    return false;
}

static bool start_host(void *context, const char *url, char *error, size_t size) {
    MockEndpoint *ep = context;
    SwmpMock *mock = ep->mock;
    (void)url;  // one simulated rendezvous per mock
    if (ep->running || mock->host != SWMP_MOCK_ENDPOINTS)
        return start_error(error, size, "Mock host already running");
    snprintf(mock->invite, sizeof mock->invite, "mock-%u", ++mock->generation);
    mock->host = ep->index;
    ep->running = ep->invite_pending = true;
    return true;
}

static bool start_join(void *context, const char *url, const char *invite, char *error, size_t size) {
    MockEndpoint *ep = context;
    SwmpMock *mock = ep->mock;
    (void)url;
    if (ep->running || mock->host == SWMP_MOCK_ENDPOINTS || !invite || strcmp(invite, mock->invite))
        return start_error(error, size, "Mock invite unavailable");
    for (size_t i = 0; i < SWMP_MOCK_ENDPOINTS - 1; i++) {
        MockLink *link = &mock->links[i];
        if (link->id || mock->last_connection == UINT32_MAX) continue;
        *link = (MockLink){
            .id = ++mock->last_connection,
            .endpoints = {mock->host, ep->index},
            .open = true,
            .pending = {true, true},
        };
        ep->running = true;
        return true;
    }
    return start_error(error, size, "Mock connection limit reached");
}

static bool poll(void *context, SwmpTransportEvent *event) {
    MockEndpoint *ep = context;
    SwmpMock *mock = ep->mock;
    free(ep->polled_data);
    ep->polled_data = NULL;
    if (!ep->running) return false;
    if (ep->invite_pending) {
        ep->invite_pending = false;
        memcpy(ep->polled_text, mock->invite, sizeof ep->polled_text);
        *event = (SwmpTransportEvent){.type = SWMP_TRANSPORT_INVITE_READY, .text = ep->polled_text};
        return true;
    }
    for (size_t i = 0; i < SWMP_MOCK_ENDPOINTS - 1; i++) {
        MockLink *link = &mock->links[i];
        int side = link_side(link, ep);
        if (side < 0 || !link->pending[side]) continue;
        *event = (SwmpTransportEvent){
            .type = link->open ? SWMP_TRANSPORT_PEER_CONNECTED : SWMP_TRANSPORT_PEER_DISCONNECTED,
            .connection = link->id,
        };
        if (link->open) {
            // Predictable fixture bytes. Production bindings come from the authenticated transport.
            for (unsigned j = 0; j < 4; j++) event->binding[j] = (uint8_t)(link->id >> (24 - 8 * j));
        }
        link->pending[side] = false;
        reap_link(link);
        return true;
    }
    if (!ep->count) return false;
    MockPacket packet = remove_packet(ep, 0);
    ep->polled_data = packet.data;
    *event = (SwmpTransportEvent){
        .type = SWMP_TRANSPORT_PACKET,
        .connection = packet.connection,
        .channel = packet.channel,
        .data = packet.data,
        .size = packet.size,
    };
    return true;
}

static void insert_packet(MockEndpoint *ep, size_t index, MockPacket packet) {
    memmove(&ep->queue[index + 1], &ep->queue[index], (ep->count - index) * sizeof packet);
    ep->queue[index] = packet;
    ep->count++;
}

static bool send(void *context, SwmpConnection id, SwmpChannel channel, const uint8_t *data, size_t size) {
    MockEndpoint *ep = context;
    MockLink *link = find_link(ep, id);
    if (!link || (unsigned)channel >= SWMP_CHANNEL_COUNT || !data || !size ||
        size > SWMP_PACKET_HEADER_SIZE + SWMP_MAX_PAYLOAD_SIZE)
        return false;
    MockEndpoint *to = &ep->mock->endpoints[link->endpoints[1 - link_side(link, ep)]];
    SwmpMockFault fault = SWMP_MOCK_NORMAL;
    if (channel == SWMP_CHANNEL_STATE) {
        fault = ep->fault;
        ep->fault = SWMP_MOCK_NORMAL;
        size_t copies = fault == SWMP_MOCK_DUPLICATE_STATE ? 2 : 1;
        if (fault == SWMP_MOCK_DROP_STATE || SWMP_MOCK_QUEUE_SIZE - to->count < copies) return true;
    }

    size_t evict = to->count;
    if (to->count == SWMP_MOCK_QUEUE_SIZE) {
        for (size_t i = 0; i < to->count; i++) {
            if (to->queue[i].channel != SWMP_CHANNEL_STATE) continue;
            evict = i;
            break;
        }
        if (evict == to->count) return false;
    }
    uint8_t *copy = malloc(size);
    uint8_t *duplicate = fault == SWMP_MOCK_DUPLICATE_STATE ? malloc(size) : NULL;
    if (!copy || (fault == SWMP_MOCK_DUPLICATE_STATE && !duplicate)) {
        free(copy);
        free(duplicate);
        return false;
    }
    memcpy(copy, data, size);
    if (evict < to->count) free(remove_packet(to, evict).data);
    size_t index = to->count;
    if (fault == SWMP_MOCK_REORDER_STATE) {
        for (size_t i = to->count; i > 0; i--) {
            if (to->queue[i - 1].channel != SWMP_CHANNEL_STATE || to->queue[i - 1].connection != id) continue;
            index = i - 1;
            break;
        }
    }
    MockPacket packet = {.connection = id, .channel = channel, .size = size, .data = copy};
    insert_packet(to, index, packet);
    if (duplicate) {
        memcpy(duplicate, data, size);
        packet.data = duplicate;
        insert_packet(to, to->count, packet);
    }
    return true;
}

static void disconnect(void *context, SwmpConnection id) {
    MockEndpoint *ep = context;
    MockLink *link = find_link(ep, id);
    if (link) close_link(ep->mock, link);
}

static void stop(void *context) {
    MockEndpoint *ep = context;
    SwmpMock *mock = ep->mock;
    ep->running = ep->invite_pending = false;
    ep->fault = SWMP_MOCK_NORMAL;
    if (mock->host == ep->index) mock->host = SWMP_MOCK_ENDPOINTS;
    for (size_t i = 0; i < SWMP_MOCK_ENDPOINTS - 1; i++) {
        MockLink *link = &mock->links[i];
        int side = link_side(link, ep);
        if (side < 0) continue;
        if (link->open) close_link(mock, link);
        else {
            link->pending[side] = false;
            reap_link(link);
        }
    }
    while (ep->count) free(remove_packet(ep, ep->count - 1).data);
    free(ep->polled_data);
    ep->polled_data = NULL;
}

SwmpMock *swmp_mock_create(void) {
    SwmpMock *mock = calloc(1, sizeof *mock);
    if (!mock) return NULL;
    mock->host = SWMP_MOCK_ENDPOINTS;
    for (unsigned i = 0; i < SWMP_MOCK_ENDPOINTS; i++) {
        mock->endpoints[i].mock = mock;
        mock->endpoints[i].index = i;
    }
    return mock;
}

void swmp_mock_destroy(SwmpMock *mock) {
    if (!mock) return;
    for (unsigned i = 0; i < SWMP_MOCK_ENDPOINTS; i++) stop(&mock->endpoints[i]);
    free(mock);
}

SwmpTransport swmp_mock_transport(SwmpMock *mock, unsigned endpoint) {
    if (!mock || endpoint >= SWMP_MOCK_ENDPOINTS) return (SwmpTransport){0};
    return (SwmpTransport){
        .context = &mock->endpoints[endpoint],
        .start_host = start_host,
        .start_join = start_join,
        .poll = poll,
        .send = send,
        .disconnect = disconnect,
        .stop = stop,
    };
}

void swmp_mock_fault(SwmpMock *mock, unsigned endpoint, SwmpMockFault next_state_send) {
    if (mock && endpoint < SWMP_MOCK_ENDPOINTS) mock->endpoints[endpoint].fault = next_state_send;
}
