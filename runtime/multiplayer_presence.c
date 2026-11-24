#include "cpu.h"

#ifdef SW_MULTIPLAYER
#include "framepace.h"
#include "game_place.h"
#include "game_sites.h"
#include "hle.h"
#include "kernel.h"
#include "multiplayer.h"
#include "multiplayer_assets.h"
#include "settings.h"
#include "world_environment.h"

#include <math.h>

// Per slot: native creation request, retained actor index, temporary ordinary actor link.
enum { INDEX_OFFSET = 0x10, LINK_OFFSET = 0x20, SLOT_SIZE = 0x40 };
static const uint64_t retry_ns = UINT64_C(1000000000);
static const uint64_t request_ns = UINT64_C(10000000000);
static const char placeholder_name[] = "Npc_TripMaster_00";
static const float pi = 3.14159265358979323846f;

typedef struct {
    float position[3], yaw;
    SwmpProxyAction action;
} Pose;

typedef struct {
    uint64_t generation, deadline;
    uint64_t move_started;
    Pose applied, from, target;
    SwmpProxyAction animation;
    bool retained, visible, proxy;
} Placeholder;

typedef struct {
    uint64_t deadline;
    SwmpGear gear;
    bool retained, visible;
} GearActor;

// Owned by the guest menu update thread. The request address must stay fixed while the game owns it.
static Placeholder slots[SWMP_MAX_PEERS];
static GearActor gear_slots[SWMP_GEAR_SLOTS][SWMP_MAX_PEERS];
static uint64_t storage, gear_storage[SWMP_GEAR_SLOTS], lobby_generation;
static uint64_t local_context, local_actor;
static uint32_t local_area;
static uint64_t applied_clock_at;
static int16_t local_yaw;
static uint64_t local_motion_at;
static float local_motion_x, local_motion_z;
static SwmpProxyAction local_motion_action;

RT_ON_RESET(multiplayer_presence_reset) {
    storage = lobby_generation = 0;
    local_context = local_actor = 0;
    local_area = 0;
    applied_clock_at = 0;
    local_yaw = 0;
    local_motion_at = 0;
    memset(slots, 0, sizeof slots);
    memset(gear_slots, 0, sizeof gear_slots);
    memset(gear_storage, 0, sizeof gear_storage);
}

static bool guest_range(uint64_t addr, size_t size) {
    return addr >= GUEST_LO && addr < GUEST_HI && size <= GUEST_HI - addr;
}

static uint64_t call(Cpu *source, uint64_t fn, uint64_t arg0, uint64_t arg1, uint64_t arg2) {
    Cpu cpu = *source;
    cpu.x[0] = arg0;
    cpu.x[1] = arg1;
    cpu.x[2] = arg2;
    guest_call(&cpu, fn);
    return cpu.x[0];
}

static uint64_t resolve(Cpu *cpu, uint64_t addr) {
    uint64_t actor = call(cpu, g_site.actor_index_resolve, addr + INDEX_OFFSET, addr + LINK_OFFSET, 0);
    return guest_range(actor, 0x7c) && LD64(addr + LINK_OFFSET + 8) == actor ? actor : 0;
}

static void release_actor(Cpu *cpu, uint64_t addr, bool retained) {
    if (retained) {
        uint64_t actor = resolve(cpu, addr);
        if (actor) call(cpu, g_site.mp_presence_delete, actor, 2, 0);
        call(cpu, g_site.actor_link_release, addr + LINK_OFFSET, 0, 0);
        call(cpu, g_site.actor_index_release, addr + INDEX_OFFSET, 0, 0);
    }
    call(cpu, g_site.mp_presence_release, addr, 0, 0);
}

static uint64_t actor_slots_alloc(void) {
    uint64_t addr = (uintptr_t)hle_alloc(SWMP_MAX_PEERS * SLOT_SIZE);
    if (!addr) return 0;
    memset((void *)(uintptr_t)addr, 0, SWMP_MAX_PEERS * SLOT_SIZE);
    for (uint8_t peer = 0; peer < SWMP_MAX_PEERS; peer++)
        ST32(addr + peer * SLOT_SIZE + INDEX_OFFSET + 8, UINT32_MAX);
    return addr;
}

static void retire_gear(Cpu *cpu, uint8_t peer, SwmpGearSlot kind, uint64_t now) {
    GearActor *slot = &gear_slots[kind][peer];
    release_actor(cpu, gear_storage[kind] + peer * SLOT_SIZE, slot->retained);
    LOG("[multiplayer] remote %s %s for peer %u\n", swmp_gear_assets[slot->gear].label,
        slot->visible ? "removed" : "request retired", peer);
    *slot = (GearActor){.deadline = now + retry_ns};
}

static void retire(Cpu *cpu, uint8_t peer, uint64_t now, const char *reason) {
    Placeholder *slot = &slots[peer];
    for (unsigned kind = 0; kind < SWMP_GEAR_SLOTS; kind++)
        if (gear_slots[kind][peer].gear) retire_gear(cpu, peer, kind, now);
    release_actor(cpu, storage + peer * SLOT_SIZE, slot->retained);
    LOG("[multiplayer] %s %s for peer %u (%s)\n", slot->proxy ? "remote Link" : "placeholder",
        slot->visible ? "removed" : "request retired", peer, reason);
    // Native deletion runs asynchronously. Give its job time before using this placement again.
    *slot = (Placeholder){.deadline = now + retry_ns};
}

static uint64_t spawn_context(void) {
    if (!guest_range(g_site.mp_presence_context, 8)) return 0;
    uint64_t owner = LD64(g_site.mp_presence_context);
    if (!guest_range(owner, 0x30)) return 0;
    uint64_t context = LD64(owner + 0x28);
    return guest_range(context, 8) ? context : 0;
}

// Native actors stayed active on both peers at 60 m and were culled at 92 m.
// Spawn inside the observed range and leave room for movement before retiring.
static const float spawn_radius = 50.0f, retain_radius = 60.0f;

static bool valid_position(const float pos[3]) {
    return fabsf(pos[0]) <= 5000 && fabsf(pos[1]) <= 2000 && fabsf(pos[2]) <= 4000;
}

static bool heap_object(uint64_t addr, uint64_t heap_size, size_t size) {
    return !(addr & 7) && addr >= HEAP_BASE && heap_size >= size && addr - HEAP_BASE <= heap_size - size;
}

static uint64_t animation_controller(uint64_t actor, uint64_t heap_size) {
    // The generic actor owns its AS controller. It becomes available after the model is loaded.
    if (!heap_object(actor, heap_size, 0x568)) return 0;
    uint64_t controller = LD64(actor + 0x560);
    return heap_object(controller, heap_size, 0xe0) ? controller : 0;
}

static bool action_named(uint64_t name, const char *expected) {
    size_t size = strlen(expected) + 1;
    return guest_range(name, size) && !memcmp((const void *)(uintptr_t)name, expected, size);
}

static SwmpProxyAction sample_action(Cpu *cpu, uint64_t actor, uint64_t heap_size, SwmpGear weapon,
                                     bool drawn, bool shield_raised) {
    if (!g_site.mp_anim_current) return SWMP_PROXY_IDLE;
    uint64_t controller = animation_controller(actor, heap_size);
    if (!controller) return SWMP_PROXY_IDLE;
    // Layer zero, slot zero holds the local actor's full-body state.
    uint64_t string = call(cpu, g_site.mp_anim_current, controller, 0, 0);
    if (!guest_range(string, 16)) return SWMP_PROXY_IDLE;
    uint64_t name = LD64(string + 8);
    if (!guest_range(name, 64)) return SWMP_PROXY_IDLE;
    const char *state = (const char *)(uintptr_t)name;
    return memchr(state, 0, 64) ? swmp_proxy_sample_action(state, weapon, drawn, shield_raised) : SWMP_PROXY_IDLE;
}

static SwmpProxyAction sample_locomotion(const float pos[3], uint64_t now) {
    uint64_t elapsed = now - local_motion_at;
    if (local_motion_at && elapsed < UINT64_C(100000000)) return local_motion_action;

    SwmpProxyAction action = SWMP_PROXY_WALK;
    if (local_motion_at && elapsed <= UINT64_C(300000000)) {
        // The native Move state blends walking, running and sprinting. Measure horizontal speed over
        // a short window of game updates. The lower exit thresholds avoid switching clips at the boundary.
        float speed = hypotf(pos[0] - local_motion_x, pos[2] - local_motion_z) / (elapsed * 1e-9f);
        if (speed < 10.0f) {
            if (speed >= (local_motion_action == SWMP_PROXY_DASH ? 5.7f : 6.2f)) action = SWMP_PROXY_DASH;
            else if (speed >= (local_motion_action == SWMP_PROXY_WALK ? 3.4f : 2.9f)) action = SWMP_PROXY_RUN;
        }
    }
    local_motion_at = now;
    local_motion_x = pos[0];
    local_motion_z = pos[2];
    return local_motion_action = action;
}

static void sample_facing(uint64_t actor, uint64_t heap_size, const float pos[3]) {
    if (!heap_object(actor, heap_size, 0x3c8)) return;
    float matrix[12];
    memcpy(matrix, (const void *)(uintptr_t)(actor + 0x398), sizeof matrix);
    // Confirm the published actor transform belongs to the current player placement.
    for (unsigned i = 0; i < 3; i++)
        if (!(fabsf(matrix[3 + 4 * i] - pos[i]) < 2.0f)) return;
    float x = matrix[2], z = matrix[10], length_sq = x * x + z * z;
    if (!(length_sq > 0.5f && length_sq < 1.5f)) return;
    long turn = lround(atan2((double)x, (double)z) * (32768.0 / pi));
    local_yaw = (int16_t)(turn == 32768 ? -32768 : turn);
}

static int16_t quantize_turn(float angle) {
    long turn = lroundf(remainderf(angle, 2 * pi) * (32768.0f / pi));
    return (int16_t)(turn == 32768 ? -32768 : turn);
}

static bool sample_gear_pose(uint64_t item, const float pos[3], int16_t facing, SwmpGearPose *pose) {
    float a[12];
    memcpy(a, (const void *)(uintptr_t)(item + 0x398), sizeof a);
    for (unsigned i = 0; i < 12; i++)
        if (!isfinite(a[i])) return false;
    float forward = a[2] * a[2] + a[6] * a[6] + a[10] * a[10];
    if (!(forward > 0.5f && forward < 1.5f)) return false;

    float yaw = facing * (pi / 32768.0f), c = cosf(yaw), s = sinf(yaw);
    float dx = a[3] - pos[0], dy = a[7] - pos[1], dz = a[11] - pos[2];
    float offset[3] = {c * dx - s * dz, dy, s * dx + c * dz};
    for (unsigned i = 0; i < 3; i++)
        if (fabsf(offset[i]) > SWMP_GEAR_OFFSET_LIMIT_MM / 1000.0f) return false;
    for (unsigned i = 0; i < 3; i++) pose->offset_mm[i] = (int16_t)lroundf(1000 * offset[i]);
    // The same Y-X-Z convention is used to reconstruct the native matrix on the receiver.
    float horizontal = hypotf(a[2], a[10]);
    pose->rotation[0] = quantize_turn(atan2f(-a[6], horizontal));
    pose->rotation[1] = quantize_turn((horizontal > 1e-4f ? atan2f(a[2], a[10]) : atan2f(-a[8], a[0])) - yaw);
    pose->rotation[2] = horizontal > 1e-4f ? quantize_turn(atan2f(a[4], a[5])) : 0;
    return true;
}

static SwmpGearSet sample_gear(Cpu *cpu, uint64_t actor, uint64_t heap_size, const float pos[3],
                               SwmpPlayerSnapshot *sample) {
    SwmpGearSet gear = {0};
    if (!g_site.player_equipment) return gear;
    // The equipment getter uses the same category ordering as the equipped-item drop path.
    static const uint8_t categories[SWMP_GEAR_SLOTS] = {[SWMP_GEAR_WEAPON] = 0, [SWMP_GEAR_SHIELD] = 1};
    for (unsigned slot = 0; slot < SWMP_GEAR_SLOTS; slot++) {
        uint64_t item = call(cpu, g_site.player_equipment, actor, categories[slot], 0);
        if (!heap_object(item, heap_size, 0x3c8) || LD8(item + 0x64) != 1 || (LD32(item + 0x78) & 1)) continue;
        uint64_t name = LD64(item + 0x10);
        for (unsigned id = 1; id < SWMP_GEAR_COUNT; id++) {
            const SwmpGearAsset *asset = &swmp_gear_assets[id];
            if (asset->slot != slot || !action_named(name, asset->model)) continue;
            gear.id[slot] = id;
            if (sample_gear_pose(item, pos, sample->rotation[1], &sample->gear_pose[slot]))
                sample->flags |= swmp_gear_pose_flag(slot);
            break;
        }
    }
    return gear;
}

static SwmpPlayerSnapshot sample_player(Cpu *cpu, uint64_t now, uint64_t *player, SwmpGearSet *gear) {
    SwmpPlayerSnapshot sample = {0};
    if (!g_site.player_info || framepace_title_ui() || framepace_loading_ui()) return sample;
    uint32_t area = game_place_area_id();
    if (!area) return sample;
    uint64_t info = LD64(g_site.player_info), heap_size = kernel_heap_size();
    if (!heap_object(info, heap_size, 0x94)) return sample;
    uint64_t actor = LD64(info + 0x60);
    if (!heap_object(actor, heap_size, 0x7c) || LD8(actor + 0x64) != 1 || (LD32(actor + 0x78) & 1))
        return sample;
    // PlayerInfo publishes this copy every game update. The writable position chain can change during gameplay.
    float p[3];
    memcpy(p, (const void *)(uintptr_t)(info + 0x88), sizeof p);
    if (!valid_position(p)) return sample;
    if (local_actor != actor || local_area != area) {
        local_yaw = 0;
        local_motion_at = 0;
    }
    for (unsigned i = 0; i < 3; i++) sample.position_mm[i] = (int32_t)lround((double)p[i] * 1000);
    sample_facing(actor, heap_size, p);
    sample.rotation[1] = local_yaw;
    *gear = sample_gear(cpu, actor, heap_size, p, &sample);
    // Native Wait covers both grips. For the axe, distinguish its hand and back attachments
    // using their measured height bands. The action mapper applies this only to idle states.
    int weapon_height = sample.gear_pose[SWMP_GEAR_WEAPON].offset_mm[1];
    bool drawn = (sample.flags & SWMP_PLAYER_WEAPON_POSE) && weapon_height >= 550 && weapon_height <= 1050;
    // WaitAttention also covers lock-on. A raised shield is in front of Link in his yaw frame.
    bool shield_raised = (sample.flags & SWMP_PLAYER_SHIELD_POSE) &&
        sample.gear_pose[SWMP_GEAR_SHIELD].offset_mm[2] >= 175;
    sample.action = sample_action(cpu, actor, heap_size, gear->id[SWMP_GEAR_WEAPON], drawn, shield_raised);
    if (sample.action == SWMP_PROXY_CROUCH_IDLE || sample.action == SWMP_PROXY_CROUCH_MOVE)
        sample.flags |= SWMP_PLAYER_CROUCHING;
    if (sample.action == SWMP_PROXY_SHIELD_GUARD) sample.flags |= SWMP_PLAYER_GUARDING;
    if (sample.action == SWMP_PROXY_WALK) sample.action = sample_locomotion(p, now);
    else local_motion_at = 0;
    if (g_site.mp_health_current) {
        uint32_t life = (uint32_t)call(cpu, g_site.mp_health_current, info, 0, 0);
        if (life <= UINT16_MAX) {
            sample.health_q4 = (uint16_t)life;
            if (!life) sample.flags |= SWMP_PLAYER_DEAD;
        }
    }
    // A RomFS worker can observe a stage change while sampling the player.
    if (game_place_area_id() != area) return (SwmpPlayerSnapshot){0};
    *player = actor;
    sample.area_id = area;
    return sample;
}

static bool remote_pose(const SwmpPosition *p, uint32_t area, uint64_t now_ms, Pose *pose) {
    if (p->snapshot.area_id != area || !swmp_position_fresh(p, now_ms)) return false;
    float *pos = pose->position;
    for (unsigned i = 0; i < 3; i++) pos[i] = p->snapshot.position_mm[i] / 1000.0f;
    pose->yaw = p->snapshot.rotation[1] * (pi / 32768.0f);
    pose->action = p->snapshot.action < SWMP_PROXY_ACTION_COUNT ? p->snapshot.action : SWMP_PROXY_IDLE;
    return valid_position(pos);
}

static bool update_environment(Cpu *cpu, const SwmpM1Roster *roster, uint32_t area, uint64_t now_ms,
                               SwmpEnvironment *sample) {
    if (!area || !settings_multiplayer_sync_environment() || framepace_in_event()) {
        applied_clock_at = 0;
        return false;
    }
    float tod;
    if (roster->host) {
        if (!world_time_read(&tod) || !(tod >= 0 && tod < 360)) return false;
        *sample = (SwmpEnvironment){.time_milli = (uint32_t)(tod * 1000.0), .weather = world_weather_read(cpu)};
        return true;
    }

    const SwmpHostEnvironment *host = &roster->environment;
    if (host->area_id != area || now_ms - host->received_at >= SWMP_POSITION_TIMEOUT_MS) return false;
    if (applied_clock_at != host->received_at && world_time_read(&tod)) {
        float target = host->state.time_milli / 1000.0f;
        float gap = fabsf(target - tod);
        // Leave the native clock running between corrections. Time wraps at midnight.
        if (gap <= 0.025f || (gap >= 359.975f && gap <= 360.0f) || world_time_sync(target))
            applied_clock_at = host->received_at;
    }
    if (host->state.weather < SWMP_WEATHER_TYPES) world_weather_sync(host->state.weather);
    return false;
}

static float distance_squared(const SwmpPlayerSnapshot *local, const Pose *pose) {
    float distance_sq = 0;
    for (unsigned i = 0; i < 3; i++) {
        float d = pose->position[i] - local->position_mm[i] / 1000.0f;
        distance_sq += d * d;
    }
    return distance_sq;
}

static void copy_transform(uint64_t dst, const Pose *pose) {
    float s = sinf(pose->yaw), c = cosf(pose->yaw);
    const float *pos = pose->position;
    float matrix[12] = {c, 0, s, pos[0], 0, 1, 0, pos[1], -s, 0, c, pos[2]};
    memcpy((void *)(uintptr_t)dst, matrix, sizeof matrix);
}

static bool request_actor(Cpu *source, uint64_t addr, uint64_t context, const Pose *pose, const char *actor_name) {
    Cpu cpu = *source;
    cpu.sp -= 0x80;
    uint64_t name = cpu.sp + 0x10, transform = cpu.sp + 0x30;
    memcpy((void *)(uintptr_t)name, actor_name, strlen(actor_name) + 1);
    copy_transform(transform, pose);
    cpu.x[0] = name;
    cpu.x[1] = transform;
    cpu.x[2] = context;
    cpu.x[3] = addr;
    cpu.x[4] = UINT32_MAX;  // no life override or placement ID
    cpu.x[5] = 0;
    cpu.x[6] = 1;
    cpu.v[0] = UINT32_C(0x3f800000);  // unit scale in s0
    guest_call(&cpu, g_site.mp_presence_create);
    return (cpu.x[0] & 1) != 0;
}

static void publish_matrix(Cpu *cpu, uint64_t actor) {
    cpu->x[0] = actor;
    cpu->x[1] = cpu->sp + 0x10;
    cpu->x[2] = 1; // Publish the matrix immediately to the native model.
    cpu->x[3] = 0;
    guest_call(cpu, g_site.mp_presence_move);
}

static void move_actor(Cpu *source, uint64_t actor, const Pose *pose) {
    Cpu cpu = *source;
    cpu.sp -= 0x40;
    copy_transform(cpu.sp + 0x10, pose);
    publish_matrix(&cpu, actor);
}

static void move_gear(Cpu *source, uint64_t actor, const Pose *pose, const SwmpGearPose *gear) {
    const float unit = pi / 32768.0f;
    float c = cosf(pose->yaw), s = sinf(pose->yaw);
    float x = (c * gear->offset_mm[0] + s * gear->offset_mm[2]) / 1000.0f + pose->position[0];
    float y = gear->offset_mm[1] / 1000.0f + pose->position[1];
    float z = (-s * gear->offset_mm[0] + c * gear->offset_mm[2]) / 1000.0f + pose->position[2];
    float pitch = gear->rotation[0] * unit, yaw = pose->yaw + gear->rotation[1] * unit;
    float roll = gear->rotation[2] * unit;
    float cp = cosf(pitch), sp = sinf(pitch), cy = cosf(yaw), sy = sinf(yaw), cr = cosf(roll), sr = sinf(roll);
    float matrix[12] = {
        cy*cr + sy*sp*sr, -cy*sr + sy*sp*cr, sy*cp, x,
        cp*sr, cp*cr, -sp, y,
        -sy*cr + cy*sp*sr, sy*sr + cy*sp*cr, cy*cp, z,
    };
    Cpu cpu = *source;
    cpu.sp -= 0x40;
    memcpy((void *)(uintptr_t)(cpu.sp + 0x10), matrix, sizeof matrix);
    publish_matrix(&cpu, actor);
}

static bool retain_actor(Cpu *cpu, uint64_t addr) {
    // Taking the completed actor clears its creation suspension.
    uint64_t actor = call(cpu, g_site.mp_presence_take, addr, 0, 0);
    if (!guest_range(actor, 0x7c)) return false;
    if (call(cpu, g_site.actor_index_assign, addr + INDEX_OFFSET, actor, 1)) return true;
    call(cpu, g_site.mp_presence_delete, actor, 2, 0);
    return false;
}

static void update_gear(Cpu *cpu, uint8_t peer, SwmpGearSlot kind, uint64_t now, uint64_t context,
                        const SwmpM1Roster *roster) {
    const Placeholder *link = &slots[peer];
    GearActor *slot = &gear_slots[kind][peer];
    const SwmpPlayerSnapshot *sample = &roster->positions[peer].snapshot;
    const SwmpEquipment *gear = &roster->equipment[peer];
    SwmpGear id = gear->gear.id[kind];
    if (slot->gear && slot->gear != id) {
        retire_gear(cpu, peer, kind, now);
        return;
    }
    if (!link->visible || !link->proxy || !fs_multiplayer_gear_ready(id) ||
        gear->revision != sample->equipment_revision) return;
    if (!(sample->flags & swmp_gear_pose_flag(kind))) {
        if (slot->gear) retire_gear(cpu, peer, kind, now);
        return;
    }
    if (!gear_storage[kind]) {
        gear_storage[kind] = actor_slots_alloc();
        if (!gear_storage[kind]) return;
    }
    uint64_t addr = gear_storage[kind] + peer * SLOT_SIZE;
    const SwmpGearAsset *asset = &swmp_gear_assets[id];
    if (!slot->gear) {
        if (now < slot->deadline) return;
        slot->gear = id;
        slot->deadline = now + request_ns;
        if (!request_actor(cpu, addr, context, &link->applied, asset->actor))
            retire_gear(cpu, peer, kind, now);
    } else if (slot->retained) {
        uint64_t actor = resolve(cpu, addr);
        bool active = actor && LD8(actor + 0x64) == 1 && !(LD32(actor + 0x78) & 1);
        if (active) {
            move_gear(cpu, actor, &link->applied, &sample->gear_pose[kind]);
            if (!slot->visible) LOG("[multiplayer] remote %s ready for peer %u\n", asset->label, peer);
            slot->visible = true;
        }
        call(cpu, g_site.actor_link_release, addr + LINK_OFFSET, 0, 0);
        if (!active && (slot->visible || now >= slot->deadline)) retire_gear(cpu, peer, kind, now);
    } else if (call(cpu, g_site.mp_presence_ready, addr, 0, 0) & 1) {
        slot->retained = retain_actor(cpu, addr);
        if (!slot->retained) retire_gear(cpu, peer, kind, now);
    } else if (now >= slot->deadline || (call(cpu, g_site.mp_presence_failed, addr, 0, 0) & 1)) {
        retire_gear(cpu, peer, kind, now);
    }
}

static void animate_proxy(Cpu *source, uint64_t actor, Placeholder *slot, SwmpProxyAction action) {
    if (!g_site.mp_anim_play || !g_site.mp_anim_string || slot->animation == action) return;
    uint64_t controller = animation_controller(actor, kernel_heap_size());
    if (!controller) return;

    Cpu cpu = *source;
    cpu.sp -= 0x40;
    uint64_t name = cpu.sp + 0x20;
    const char *state = swmp_proxy_action_name(action);
    memcpy((void *)(uintptr_t)name, state, strlen(state) + 1);
    ST64(cpu.sp, g_site.mp_anim_string + 0x10);  // SafeString vtable address point
    ST64(cpu.sp + 8, name);
    cpu.x[0] = controller;
    cpu.x[1] = cpu.sp;
    cpu.x[2] = cpu.x[3] = 0;  // layer and slot
    cpu.x[4] = 1;
    cpu.v[0] = cpu.v[1] = UINT32_C(0xbf800000);  // default transition and rate
    guest_call(&cpu, g_site.mp_anim_play);
    slot->animation = action;
}

static bool same_pose(const Pose *a, const Pose *b) {
    return a->yaw == b->yaw && !memcmp(a->position, b->position, sizeof a->position);
}

static void update_proxy(Cpu *cpu, uint64_t actor, Placeholder *slot, uint64_t now, const Pose *pose) {
    const uint64_t interval = SWMP_POSITION_INTERVAL_MS * UINT64_C(1000000);
    if (!same_pose(&slot->target, pose)) {
        float distance_sq = 0;
        for (unsigned i = 0; i < 3; i++) {
            float d = pose->position[i] - slot->applied.position[i];
            distance_sq += d * d;
        }
        slot->from = slot->applied;
        slot->target = *pose;
        // Interpolate walking and jumps over one send interval. Snap across multi-metre corrections.
        slot->move_started = distance_sq > 9 ? now - interval : now;
    }
    if (same_pose(&slot->applied, &slot->target)) return;
    uint64_t elapsed = now - slot->move_started;
    Pose next = slot->target;
    if (elapsed < interval) {
        float t = (float)elapsed / (float)interval;
        for (unsigned i = 0; i < 3; i++)
            next.position[i] = slot->from.position[i] + t * (slot->target.position[i] - slot->from.position[i]);
        float turn = remainderf(slot->target.yaw - slot->from.yaw, 2 * pi);
        next.yaw = remainderf(slot->from.yaw + t * turn, 2 * pi);
    }
    if (!same_pose(&slot->applied, &next)) {
        move_actor(cpu, actor, &next);
        slot->applied = next;
    }
}

static void poll_actor(Cpu *cpu, uint8_t peer, uint64_t now, const Pose *pose) {
    Placeholder *slot = &slots[peer];
    uint64_t addr = storage + peer * SLOT_SIZE;
    if (slot->retained) {
        uint64_t actor = resolve(cpu, addr);
        bool active = actor && LD8(actor + 0x64) == 1 && !(LD32(actor + 0x78) & 1);
        if (active && slot->proxy) {
            update_proxy(cpu, actor, slot, now, pose);
            animate_proxy(cpu, actor, slot, pose->action);
        }
        call(cpu, g_site.actor_link_release, addr + LINK_OFFSET, 0, 0);
        if (active && !slot->visible) {
            slot->visible = true;
            LOG("[multiplayer] %s ready for peer %u\n", slot->proxy ? "remote Link" : "placeholder", peer);
        } else if (!active && (slot->visible || now >= slot->deadline)) {
            retire(cpu, peer, now, "native actor inactive");
        }
    } else if (call(cpu, g_site.mp_presence_ready, addr, 0, 0) & 1) {
        slot->retained = retain_actor(cpu, addr);
        if (!slot->retained) retire(cpu, peer, now, "actor index unavailable");
    } else if (now >= slot->deadline || (call(cpu, g_site.mp_presence_failed, addr, 0, 0) & 1)) {
        retire(cpu, peer, now, "creation failed or timed out");
    }
}

#endif

void rt_mp_presence_tick(Cpu *cpu) {
#ifdef SW_MULTIPLAYER
    SwmpM1Roster roster = swmp_m1_roster();
    if (!roster.lobby_generation && !storage) return;
    if (!g_site.mp_presence_create || !g_site.mp_presence_move || !g_site.actor_index_assign ||
        (cpu->sp & 15) || !guest_range(cpu->sp - 0x80, 0xe0)) return;

    uint64_t now = plat_now_ns(), now_ms = now / UINT64_C(1000000);
    uint64_t player = 0;
    SwmpGearSet gear = {0};
    SwmpPlayerSnapshot local = roster.lobby_generation ? sample_player(cpu, now, &player, &gear) : (SwmpPlayerSnapshot){0};
    uint64_t context = local.area_id ? spawn_context() : 0;
    if (!context) {
        local = (SwmpPlayerSnapshot){0};
        player = 0;
    }
    bool world_changed = local_context != context || local_actor != player || local_area != local.area_id;
    bool changed = lobby_generation != roster.lobby_generation;
    if (world_changed || changed) applied_clock_at = 0;
    SwmpEnvironment environment;
    bool sampled_environment = update_environment(cpu, &roster, local.area_id, now_ms, &environment);
    swmp_m1_publish_state(roster.lobby_generation, &local, gear, sampled_environment ? &environment : NULL, now_ms);
    if ((world_changed || changed) && roster.lobby_generation)
        LOG("[multiplayer] local area %u %s\n", local.area_id, context ? "ready" : "unavailable");
    if (world_changed) local_motion_at = 0;
    local_context = context;
    local_actor = player;
    local_area = local.area_id;
    lobby_generation = roster.lobby_generation;
    if (!storage) {
        bool any = false;
        for (uint8_t peer = 0; peer < SWMP_MAX_PEERS; peer++) any |= roster.remotes.generation[peer] != 0;
        if (!any || !context) return;
        storage = actor_slots_alloc();
        if (!storage) return;
    }

    for (uint8_t peer = 0; peer < SWMP_MAX_PEERS; peer++) {
        Placeholder *slot = &slots[peer];
        Pose pose;
        uint64_t generation = roster.remotes.generation[peer];
        bool has_pose = generation && remote_pose(&roster.positions[peer], local.area_id, now_ms, &pose);
        float distance_sq = has_pose ? distance_squared(&local, &pose) : INFINITY;
        float radius = slot->generation == generation ? retain_radius : spawn_radius;
        bool desired = context && distance_sq <= radius * radius;
        if (slot->generation && (changed || world_changed || slot->generation != generation || !desired)) {
            const char *reason = changed || slot->generation != generation ? "roster changed" :
                !context ? "local world unavailable" : world_changed ? "local world changed" :
                !has_pose ? "remote position unavailable" : "out of range";
            retire(cpu, peer, now, reason);
        } else if (slot->generation) {
            poll_actor(cpu, peer, now, &pose);
            for (unsigned kind = 0; kind < SWMP_GEAR_SLOTS; kind++)
                update_gear(cpu, peer, kind, now, context, &roster);
        } else if (desired && now >= slot->deadline) {
            uint64_t addr = storage + peer * SLOT_SIZE;
            bool proxy = fs_multiplayer_proxy_ready();
            if (!proxy && local.area_id != GAME_PLACE_MAIN_FIELD_AREA) continue;
            if (!proxy) pose.yaw = 0;
            if (request_actor(cpu, addr, context, &pose, proxy ? SWMP_PROXY_ACTOR : placeholder_name)) {
                *slot = (Placeholder){.generation = generation, .deadline = now + request_ns, .proxy = proxy,
                    .applied = pose, .target = pose, .animation = SWMP_PROXY_ACTION_COUNT};
            } else {
                retire(cpu, peer, now, "creation rejected");
            }
        }
    }
#else
    (void)cpu;
#endif
}
