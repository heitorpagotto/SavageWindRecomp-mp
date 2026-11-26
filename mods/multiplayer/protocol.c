#include "lobby_protocol.h"

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
    if ((type >= SWMP_MSG_HELLO && type <= SWMP_MSG_PONG) || type == SWMP_MSG_EQUIPMENT)
        return channel == SWMP_CHANNEL_CONTROL;
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

bool swmp_gear_set_valid(SwmpGearSet gear) {
    for (unsigned i = 0; i < SWMP_ARMOR_SLOTS; i++)
        if (gear.armor_dye[i] > SWMP_ARMOR_DYE_MAX || (!gear.armor[i] && gear.armor_dye[i])) return false;
    uint8_t weapon = gear.id[SWMP_GEAR_WEAPON];
    return (weapon == SWMP_GEAR_NONE || weapon == SWMP_GEAR_WOODCUTTERS_AXE ||
            weapon == SWMP_GEAR_TRAVELERS_SWORD || weapon == SWMP_GEAR_TRAVELERS_SPEAR ||
            weapon == SWMP_GEAR_TRAVELERS_CLAYMORE) &&
           (!gear.id[SWMP_GEAR_SHIELD] || gear.id[SWMP_GEAR_SHIELD] == SWMP_GEAR_WOODEN_SHIELD ||
            gear.id[SWMP_GEAR_SHIELD] == SWMP_GEAR_SOLDIERS_SHIELD) &&
           (!gear.id[SWMP_GEAR_BOW] || gear.id[SWMP_GEAR_BOW] == SWMP_GEAR_TRAVELERS_BOW ||
            gear.id[SWMP_GEAR_BOW] == SWMP_GEAR_SOLDIERS_BOW);
}

enum {
    GEAR_POSE_SIZE = 12,
    GEAR_POSES_AT = 44,
    NOCKED_ARROW_AT = GEAR_POSES_AT + GEAR_POSE_SIZE * SWMP_GEAR_SLOTS,
    BOW_ANIMATION_AT = NOCKED_ARROW_AT + 1,
    ANIMATION_ACTIVATION_AT = NOCKED_ARROW_AT + 2,
    ANIMATION_PHASE_AT = NOCKED_ARROW_AT + 3,
    ARROW_POSE_AT = NOCKED_ARROW_AT + 4,
    SAMPLED_MS_AT = ARROW_POSE_AT + GEAR_POSE_SIZE,
};

static bool pose_offset_valid(const SwmpGearPose *pose) {
    for (unsigned i = 0; i < 3; i++) {
        int offset = pose->offset_mm[i];
        if (offset < -SWMP_GEAR_OFFSET_LIMIT_MM || offset > SWMP_GEAR_OFFSET_LIMIT_MM) return false;
    }
    return true;
}

static bool gear_pose_valid(const SwmpPlayerSnapshot *s) {
    for (unsigned slot = 0; slot < SWMP_GEAR_SLOTS; slot++) {
        if (!(s->flags & swmp_gear_pose_flag(slot))) continue;
        if (!s->equipment_revision || !pose_offset_valid(&s->gear_pose[slot])) return false;
    }
    if (s->bow_animation >= SWMP_BOW_ANIM_COUNT ||
        (s->bow_animation && !(s->flags & SWMP_PLAYER_BOW_POSE))) return false;
    return s->nocked_arrow == SWMP_ARROW_NONE ||
           (s->nocked_arrow < SWMP_ARROW_COUNT && (s->flags & SWMP_PLAYER_BOW_POSE) &&
            pose_offset_valid(&s->arrow_pose));
}

static void put_gear_pose(uint8_t *data, const SwmpGearPose *pose) {
    for (unsigned i = 0; i < 3; i++) put16(data + 2 * i, (uint16_t)pose->offset_mm[i]);
    for (unsigned i = 0; i < 3; i++) put16(data + 6 + 2 * i, (uint16_t)pose->rotation[i]);
}

static void get_gear_pose(SwmpGearPose *pose, const uint8_t *data) {
    for (unsigned i = 0; i < 3; i++) pose->offset_mm[i] = (int16_t)get16(data + 2 * i);
    for (unsigned i = 0; i < 3; i++) pose->rotation[i] = (int16_t)get16(data + 6 + 2 * i);
}

bool swmp_player_snapshot_encode(uint8_t out[SWMP_PLAYER_SNAPSHOT_SIZE], const SwmpPlayerSnapshot *s) {
    if (!out || !s || s->peer_id >= SWMP_MAX_PEERS || (s->flags & ~SWMP_PLAYER_STATE_KNOWN)) return false;
    if (s->animation_phase && !s->animation_activation) return false;
    if (!gear_pose_valid(s)) return false;
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
    memset(out + GEAR_POSES_AT, 0, SWMP_PLAYER_SNAPSHOT_SIZE - GEAR_POSES_AT);
    for (unsigned slot = 0; slot < SWMP_GEAR_SLOTS; slot++)
        if (s->flags & swmp_gear_pose_flag(slot))
            put_gear_pose(out + GEAR_POSES_AT + GEAR_POSE_SIZE * slot, &s->gear_pose[slot]);
    out[NOCKED_ARROW_AT] = s->nocked_arrow;
    out[BOW_ANIMATION_AT] = s->bow_animation;
    out[ANIMATION_ACTIVATION_AT] = s->animation_activation;
    out[ANIMATION_PHASE_AT] = s->animation_phase;
    if (s->nocked_arrow) put_gear_pose(out + ARROW_POSE_AT, &s->arrow_pose);
    put32(out + SAMPLED_MS_AT, s->sampled_ms);
    return true;
}

bool swmp_player_snapshot_decode(SwmpPlayerSnapshot *s,
                                 const uint8_t data[SWMP_PLAYER_SNAPSHOT_SIZE]) {
    if (!s || !data || data[0] >= SWMP_MAX_PEERS || (data[1] & ~SWMP_PLAYER_STATE_KNOWN) ||
        (data[ANIMATION_PHASE_AT] && !data[ANIMATION_ACTIVATION_AT])) return false;
    SwmpPlayerSnapshot decoded = {
        .peer_id = data[0],
        .flags = data[1],
        .action = get16(data + 2),
        .area_id = get32(data + 4),
        .phase_id = get32(data + 8),
        .health_q4 = get16(data + 36),
        .stamina_q4 = get16(data + 38),
        .equipment_revision = get32(data + 40),
        .nocked_arrow = data[NOCKED_ARROW_AT],
        .bow_animation = data[BOW_ANIMATION_AT],
        .animation_activation = data[ANIMATION_ACTIVATION_AT],
        .animation_phase = data[ANIMATION_PHASE_AT],
        .sampled_ms = get32(data + SAMPLED_MS_AT),
    };
    for (int i = 0; i < 3; i++) decoded.position_mm[i] = (int32_t)get32(data + 12 + i * 4);
    for (int i = 0; i < 3; i++) decoded.rotation[i] = (int16_t)get16(data + 24 + i * 2);
    for (int i = 0; i < 3; i++) decoded.velocity_cm_s[i] = (int16_t)get16(data + 30 + i * 2);
    for (unsigned slot = 0; slot < SWMP_GEAR_SLOTS; slot++)
        if (decoded.flags & swmp_gear_pose_flag(slot))
            get_gear_pose(&decoded.gear_pose[slot], data + GEAR_POSES_AT + GEAR_POSE_SIZE * slot);
    if (decoded.nocked_arrow) get_gear_pose(&decoded.arrow_pose, data + ARROW_POSE_AT);
    if (!gear_pose_valid(&decoded)) return false;
    *s = decoded;
    return true;
}


static bool environment_valid(const SwmpEnvironment *e) {
    return e->time_milli < SWMP_DAY_MILLI &&
           (e->weather < SWMP_WEATHER_TYPES || e->weather == SWMP_WEATHER_UNAVAILABLE);
}

bool swmp_environment_encode(uint8_t out[SWMP_ENVIRONMENT_SIZE], const SwmpEnvironment *e) {
    if (!out || !e || !environment_valid(e)) return false;
    put32(out, e->time_milli);
    out[4] = e->weather;
    memset(out + 5, 0, 3);
    return true;
}

bool swmp_environment_decode(SwmpEnvironment *e, const uint8_t data[SWMP_ENVIRONMENT_SIZE]) {
    if (!e || !data || data[5] || data[6] || data[7]) return false;
    SwmpEnvironment decoded = {.time_milli = get32(data), .weather = data[4]};
    if (!environment_valid(&decoded)) return false;
    *e = decoded;
    return true;
}

enum { HELLO_FIXED_SIZE = 90, WELCOME_FIXED_SIZE = 6 };

// Reject malformed UTF-8, embedded NUL, and C0/C1 controls. Lengths count bytes, not glyphs.
static bool text_valid(const uint8_t *text, size_t size) {
    if (!size) return false;
    for (size_t i = 0; i < size;) {
        uint32_t cp = text[i++];
        unsigned extra = 0;
        if (cp >= 0xc2 && cp <= 0xdf) extra = 1;
        else if (cp >= 0xe0 && cp <= 0xef) extra = 2;
        else if (cp >= 0xf0 && cp <= 0xf4) extra = 3;
        else if (cp >= 0x80) return false;
        if (size - i < extra) return false;
        if (extra) cp &= 0x7fu >> (extra + 1);
        for (unsigned j = 0; j < extra; j++) {
            uint8_t byte = text[i++];
            if ((byte & 0xc0) != 0x80) return false;
            cp = (cp << 6) | (byte & 0x3f);
        }
        if (cp < 0x20 || (cp >= 0x7f && cp < 0xa0) ||
            (extra == 2 && cp < 0x800) || (extra == 3 && cp < 0x10000) ||
            (cp >= 0xd800 && cp <= 0xdfff) || cp > 0x10ffff)
            return false;
    }
    return true;
}

static size_t text_length(const char *text, size_t capacity) {
    size_t n = 0;
    while (n < capacity && text[n]) n++;
    return n < capacity && text_valid((const uint8_t *)text, n) ? n : 0;
}

bool swmp_player_name_valid(const char *name) {
    return name && text_length(name, SWMP_PLAYER_NAME_SIZE) != 0;
}

static bool put_text(uint8_t *out, size_t capacity, size_t *pos, const char *text, size_t limit) {
    size_t n = text_length(text, limit);
    if (!n || capacity - *pos < n + 1) return false;
    out[(*pos)++] = (uint8_t)n;
    memcpy(out + *pos, text, n);
    *pos += n;
    return true;
}

static bool get_text(char *text, size_t limit, const uint8_t *data, size_t size, size_t *pos) {
    if (*pos >= size) return false;
    size_t n = data[(*pos)++];
    if (n >= limit || size - *pos < n || !text_valid(data + *pos, n)) return false;
    memcpy(text, data + *pos, n);
    text[n] = 0;
    *pos += n;
    return true;
}

enum { EQUIPMENT_ARMOR_AT = 5 + SWMP_GEAR_SLOTS,
       EQUIPMENT_DYE_AT = EQUIPMENT_ARMOR_AT + 4 * SWMP_ARMOR_SLOTS,
       EQUIPMENT_SIZE = EQUIPMENT_DYE_AT + SWMP_ARMOR_SLOTS };

static bool client_id_valid(uint8_t id) {
    return id > SWMP_HOST_PEER && id < SWMP_MAX_PEERS;
}

bool swmp_control_encode(uint8_t *out, size_t capacity, size_t *size, SwmpMessageType type, const SwmpControl *m) {
    if (!size || (type != SWMP_MSG_START && (!out || !m))) return false;
    size_t pos = 0;
    switch (type) {
    case SWMP_MSG_HELLO: {
        const SwmpCompatibility *c = &m->hello.compatibility;
        if (capacity < HELLO_FIXED_SIZE || !c->protocol || (c->required_features & ~c->features)) return false;
        put16(out, c->protocol);
        for (int i = 0; i < 3; i++) {
            put16(out + 2 + i * 2, c->mod_version[i]);
            put16(out + 8 + i * 2, c->game_version[i]);
        }
        memcpy(out + 14, c->savage_build, sizeof c->savage_build);
        memcpy(out + 34, c->game_build, sizeof c->game_build);
        put32(out + 66, c->features);
        put32(out + 70, c->required_features);
        memcpy(out + 74, m->hello.binding, SWMP_BINDING_SIZE);
        pos = HELLO_FIXED_SIZE;
        if (!put_text(out, capacity, &pos, m->hello.name, SWMP_PLAYER_NAME_SIZE)) return false;
        break;
    }
    case SWMP_MSG_WELCOME:
        if (capacity < WELCOME_FIXED_SIZE || !client_id_valid(m->welcome.local_peer) ||
            !m->welcome.names[SWMP_HOST_PEER][0] || !m->welcome.names[m->welcome.local_peer][0])
            return false;
        out[0] = m->welcome.local_peer;
        out[1] = m->welcome.playing;
        put32(out + 2, m->welcome.features);
        pos = WELCOME_FIXED_SIZE;
        for (uint8_t i = 0; i < SWMP_MAX_PEERS; i++) {
            if (!m->welcome.names[i][0]) continue;
            if (pos == capacity) return false;
            out[pos++] = i;
            if (!put_text(out, capacity, &pos, m->welcome.names[i], SWMP_PLAYER_NAME_SIZE)) return false;
        }
        break;
    case SWMP_MSG_REJECT:
        if (!capacity || m->reject.code <= SWMP_REJECT_NONE || m->reject.code > SWMP_REJECT_INVALID) return false;
        out[pos++] = (uint8_t)m->reject.code;
        if (!put_text(out, capacity, &pos, m->reject.text, SWMP_REJECT_TEXT_SIZE)) return false;
        break;
    case SWMP_MSG_PEER_JOINED:
    case SWMP_MSG_PEER_LEFT:
        if (!capacity || !client_id_valid(m->peer.id)) return false;
        out[pos++] = m->peer.id;
        if (type == SWMP_MSG_PEER_JOINED &&
            !put_text(out, capacity, &pos, m->peer.name, SWMP_PLAYER_NAME_SIZE)) return false;
        break;
    case SWMP_MSG_START:
        break;
    case SWMP_MSG_EQUIPMENT:
        if (capacity < EQUIPMENT_SIZE || m->equipment.peer >= SWMP_MAX_PEERS ||
            !swmp_gear_set_valid(m->equipment.state.gear) || !m->equipment.state.revision) return false;
        out[0] = m->equipment.peer;
        put32(out + 1, m->equipment.state.revision);
        memcpy(out + 5, m->equipment.state.gear.id, SWMP_GEAR_SLOTS);
        for (unsigned i = 0; i < SWMP_ARMOR_SLOTS; i++)
            put32(out + EQUIPMENT_ARMOR_AT + i * 4, m->equipment.state.gear.armor[i]);
        memcpy(out + EQUIPMENT_DYE_AT, m->equipment.state.gear.armor_dye, SWMP_ARMOR_SLOTS);
        pos = EQUIPMENT_SIZE;
        break;
    case SWMP_MSG_PING:
    case SWMP_MSG_PONG:
        if (capacity < 8) return false;
        put64(out, m->ping_token);
        pos = 8;
        break;
    default:
        return false;
    }
    *size = pos;
    return true;
}

bool swmp_control_decode(SwmpControl *msg, SwmpMessageType type, const uint8_t *data, size_t size) {
    if (!msg || (!data && size) || size > SWMP_CONTROL_MAX_SIZE) return false;
    SwmpControl m = {0};
    size_t pos = 0;
    switch (type) {
    case SWMP_MSG_HELLO: {
        if (size < HELLO_FIXED_SIZE) return false;
        SwmpCompatibility *c = &m.hello.compatibility;
        c->protocol = get16(data);
        for (int i = 0; i < 3; i++) {
            c->mod_version[i] = get16(data + 2 + i * 2);
            c->game_version[i] = get16(data + 8 + i * 2);
        }
        memcpy(c->savage_build, data + 14, sizeof c->savage_build);
        memcpy(c->game_build, data + 34, sizeof c->game_build);
        c->features = get32(data + 66);
        c->required_features = get32(data + 70);
        memcpy(m.hello.binding, data + 74, SWMP_BINDING_SIZE);
        pos = HELLO_FIXED_SIZE;
        if (!c->protocol || (c->required_features & ~c->features) ||
            !get_text(m.hello.name, SWMP_PLAYER_NAME_SIZE, data, size, &pos)) return false;
        break;
    }
    case SWMP_MSG_WELCOME:
        if (size < WELCOME_FIXED_SIZE || !client_id_valid(data[0]) || data[1] > 1) return false;
        m.welcome.local_peer = data[0];
        m.welcome.playing = data[1] != 0;
        m.welcome.features = get32(data + 2);
        pos = WELCOME_FIXED_SIZE;
        while (pos < size) {
            uint8_t id = data[pos++];
            if (id >= SWMP_MAX_PEERS || m.welcome.names[id][0] ||
                !get_text(m.welcome.names[id], SWMP_PLAYER_NAME_SIZE, data, size, &pos)) return false;
        }
        if (!m.welcome.names[SWMP_HOST_PEER][0] || !m.welcome.names[m.welcome.local_peer][0]) return false;
        break;
    case SWMP_MSG_REJECT:
        if (!size || !data[0] || data[0] > SWMP_REJECT_INVALID) return false;
        m.reject.code = (SwmpRejectCode)data[pos++];
        if (!get_text(m.reject.text, SWMP_REJECT_TEXT_SIZE, data, size, &pos)) return false;
        break;
    case SWMP_MSG_PEER_JOINED:
    case SWMP_MSG_PEER_LEFT:
        if (!size || !client_id_valid(data[0])) return false;
        m.peer.id = data[pos++];
        if (type == SWMP_MSG_PEER_JOINED &&
            !get_text(m.peer.name, SWMP_PLAYER_NAME_SIZE, data, size, &pos)) return false;
        break;
    case SWMP_MSG_START:
        break;
    case SWMP_MSG_EQUIPMENT:
        if (size != EQUIPMENT_SIZE || data[0] >= SWMP_MAX_PEERS) return false;
        m.equipment.peer = data[0];
        m.equipment.state.revision = get32(data + 1);
        memcpy(m.equipment.state.gear.id, data + 5, SWMP_GEAR_SLOTS);
        for (unsigned i = 0; i < SWMP_ARMOR_SLOTS; i++)
            m.equipment.state.gear.armor[i] = get32(data + EQUIPMENT_ARMOR_AT + i * 4);
        memcpy(m.equipment.state.gear.armor_dye, data + EQUIPMENT_DYE_AT, SWMP_ARMOR_SLOTS);
        if (!m.equipment.state.revision || !swmp_gear_set_valid(m.equipment.state.gear)) return false;
        pos = size;
        break;
    case SWMP_MSG_PING:
    case SWMP_MSG_PONG:
        if (size != 8) return false;
        m.ping_token = get64(data);
        pos = 8;
        break;
    default:
        return false;
    }
    if (pos != size) return false;
    *msg = m;
    return true;
}
