#include "protocol.h"

#include <string.h>

static const uint32_t SWMP_MAGIC = 0x53574d50u;  // SWMP

static void put16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)v;
}

static void put32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

static void put64(uint8_t *p, uint64_t v) {
    put32(p, (uint32_t)(v >> 32));
    put32(p + 4, (uint32_t)v);
}

static uint16_t get16(const uint8_t *p) { return (uint16_t)((uint16_t)p[0] << 8 | p[1]); }

static uint32_t get32(const uint8_t *p) {
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
}

static uint64_t get64(const uint8_t *p) { return (uint64_t)get32(p) << 32 | get32(p + 4); }

static bool type_valid(uint8_t type) {
    switch (type) {
    case SWMP_MSG_HELLO:
    case SWMP_MSG_WELCOME:
    case SWMP_MSG_REJECT:
    case SWMP_MSG_PEER_JOINED:
    case SWMP_MSG_PEER_LEFT:
    case SWMP_MSG_START:
    case SWMP_MSG_PING:
    case SWMP_MSG_PONG:
    case SWMP_MSG_PLAYER_SNAPSHOT:
    case SWMP_MSG_EQUIPMENT:
    case SWMP_MSG_DAMAGE_REQUEST:
    case SWMP_MSG_DAMAGE_APPLIED:
    case SWMP_MSG_ACTOR_HEALTH:
    case SWMP_MSG_FLAG_TRANSACTION:
    case SWMP_MSG_CHEST_REWARD:
    case SWMP_MSG_WORLD_PHASE:
    case SWMP_MSG_RESYNC_REQUEST:
    case SWMP_MSG_RESYNC_SNAPSHOT:
        return true;
    default:
        return false;
    }
}

static bool type_channel_valid(uint8_t type, uint8_t channel) {
    if (type >= SWMP_MSG_HELLO && type <= SWMP_MSG_PONG) return channel == SWMP_CHANNEL_CONTROL;
    if (type == SWMP_MSG_PLAYER_SNAPSHOT) return channel == SWMP_CHANNEL_STATE;
    return channel == SWMP_CHANNEL_EVENT;
}

static SwmpPacketError validate(const SwmpPacketHeader *h) {
    if (!h) return SWMP_PACKET_BUFFER_TOO_SMALL;
    if (h->version != SWMP_PROTOCOL_VERSION) return SWMP_PACKET_BAD_VERSION;
    if (!type_valid(h->type)) return SWMP_PACKET_BAD_TYPE;
    if ((h->flags & ~SWMP_FLAG_KNOWN) ||
        (h->flags & (SWMP_FLAG_REQUEST | SWMP_FLAG_RESPONSE)) ==
            (SWMP_FLAG_REQUEST | SWMP_FLAG_RESPONSE))
        return SWMP_PACKET_BAD_FLAGS;
    if (h->channel >= SWMP_CHANNEL_COUNT) return SWMP_PACKET_BAD_CHANNEL;
    if (!type_channel_valid(h->type, h->channel)) return SWMP_PACKET_BAD_CHANNEL;
    if (h->sender_peer >= SWMP_MAX_PEERS &&
        !(h->sender_peer == SWMP_UNASSIGNED_PEER && h->type == SWMP_MSG_HELLO))
        return SWMP_PACKET_BAD_PEER;
    if (h->payload_size > SWMP_MAX_PAYLOAD_SIZE) return SWMP_PACKET_BAD_PAYLOAD_SIZE;
    return SWMP_PACKET_OK;
}

SwmpPacketError swmp_packet_encode(uint8_t *out, size_t out_size, const SwmpPacketHeader *h) {
    SwmpPacketError error = validate(h);
    if (error != SWMP_PACKET_OK) return error;
    size_t packet_size = SWMP_PACKET_HEADER_SIZE + (size_t)h->payload_size;
    if (!out || out_size < packet_size) return SWMP_PACKET_BUFFER_TOO_SMALL;
    put32(out, SWMP_MAGIC);
    put16(out + 4, h->version);
    out[6] = h->type;
    out[7] = h->flags;
    put32(out + 8, h->payload_size);
    put64(out + 12, h->session_id);
    out[20] = h->sender_peer;
    out[21] = h->channel;
    put16(out + 22, 0);
    put32(out + 24, h->sequence);
    put32(out + 28, h->tick);
    return SWMP_PACKET_OK;
}

SwmpPacketError swmp_packet_decode(SwmpPacketHeader *h, const uint8_t *packet, size_t packet_size) {
    if (!h || !packet || packet_size < SWMP_PACKET_HEADER_SIZE) return SWMP_PACKET_BUFFER_TOO_SMALL;
    if (get32(packet) != SWMP_MAGIC) return SWMP_PACKET_BAD_MAGIC;
    if (get16(packet + 22) != 0) return SWMP_PACKET_BAD_RESERVED;
    SwmpPacketHeader decoded = {
        .version = get16(packet + 4),
        .type = packet[6],
        .flags = packet[7],
        .payload_size = get32(packet + 8),
        .session_id = get64(packet + 12),
        .sender_peer = packet[20],
        .channel = packet[21],
        .sequence = get32(packet + 24),
        .tick = get32(packet + 28),
    };
    SwmpPacketError error = validate(&decoded);
    if (error != SWMP_PACKET_OK) return error;
    if (packet_size != SWMP_PACKET_HEADER_SIZE + (size_t)decoded.payload_size)
        return SWMP_PACKET_SIZE_MISMATCH;
    *h = decoded;
    return SWMP_PACKET_OK;
}

const char *swmp_packet_error_string(SwmpPacketError error) {
    switch (error) {
    case SWMP_PACKET_OK: return "ok";
    case SWMP_PACKET_BUFFER_TOO_SMALL: return "buffer too small";
    case SWMP_PACKET_BAD_MAGIC: return "bad magic";
    case SWMP_PACKET_BAD_VERSION: return "unsupported protocol version";
    case SWMP_PACKET_BAD_TYPE: return "unknown message type";
    case SWMP_PACKET_BAD_FLAGS: return "unknown packet flags";
    case SWMP_PACKET_BAD_CHANNEL: return "unknown channel";
    case SWMP_PACKET_BAD_PEER: return "invalid peer";
    case SWMP_PACKET_BAD_RESERVED: return "reserved header bits are set";
    case SWMP_PACKET_BAD_PAYLOAD_SIZE: return "payload too large";
    case SWMP_PACKET_SIZE_MISMATCH: return "packet size mismatch";
    }
    return "unknown packet error";
}

bool swmp_player_snapshot_encode(uint8_t out[SWMP_PLAYER_SNAPSHOT_SIZE], const SwmpPlayerSnapshot *s) {
    if (!out || !s || s->peer_id >= SWMP_MAX_PEERS || (s->flags & ~SWMP_PLAYER_STATE_KNOWN)) return false;
    out[0] = s->peer_id;
    out[1] = s->flags;
    put16(out + 2, s->action);
    put32(out + 4, s->area_id);
    put32(out + 8, s->phase_id);
    for (int i = 0; i < 3; i++) put32(out + 12 + i * 4, (uint32_t)s->position_mm[i]);
    for (int i = 0; i < 3; i++) put16(out + 24 + i * 2, (uint16_t)s->rotation[i]);
    for (int i = 0; i < 3; i++) put16(out + 30 + i * 2, (uint16_t)s->velocity_cm_s[i]);
    put16(out + 36, s->health_q4);
    put16(out + 38, s->stamina_q4);
    put32(out + 40, s->equipment_revision);
    return true;
}

bool swmp_player_snapshot_decode(SwmpPlayerSnapshot *s,
                                 const uint8_t data[SWMP_PLAYER_SNAPSHOT_SIZE]) {
    if (!s || !data || data[0] >= SWMP_MAX_PEERS || (data[1] & ~SWMP_PLAYER_STATE_KNOWN)) return false;
    SwmpPlayerSnapshot decoded = {
        .peer_id = data[0],
        .flags = data[1],
        .action = get16(data + 2),
        .area_id = get32(data + 4),
        .phase_id = get32(data + 8),
        .health_q4 = get16(data + 36),
        .stamina_q4 = get16(data + 38),
        .equipment_revision = get32(data + 40),
    };
    for (int i = 0; i < 3; i++) decoded.position_mm[i] = (int32_t)get32(data + 12 + i * 4);
    for (int i = 0; i < 3; i++) decoded.rotation[i] = (int16_t)get16(data + 24 + i * 2);
    for (int i = 0; i < 3; i++) decoded.velocity_cm_s[i] = (int16_t)get16(data + 30 + i * 2);
    *s = decoded;
    return true;
}

