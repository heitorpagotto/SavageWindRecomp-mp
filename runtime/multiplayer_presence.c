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
enum { INDEX_OFFSET = 0x10, LINK_OFFSET = 0x20, SLOT_SIZE = 0x40, PLAYBACK_SAMPLES = 8 };
static const uint64_t retry_ns = UINT64_C(1000000000);
static const uint64_t request_ns = UINT64_C(10000000000);
static const uint64_t snapshot_ns = SWMP_POSITION_INTERVAL_MS * UINT64_C(1000000);
static const uint64_t playback_delay_ms = 2 * SWMP_POSITION_INTERVAL_MS + 25;
static const char placeholder_name[] = "Npc_TripMaster_00";
static const float pi = 3.14159265358979323846f;
static const float glide_bank_limit = 0.314159265f;  // 18 degrees

typedef struct {
    float position[3], yaw, roll;
    SwmpProxyAction action;
} Pose;

typedef struct {
    SwmpPlayerSnapshot snapshot;
    uint64_t play_at;
} PlaybackSample;

typedef struct {
    uint64_t generation, deadline;
    PlaybackSample playback[PLAYBACK_SAMPLES];
    unsigned playback_count;
    Pose applied;
    float model_lift;
    SwmpProxyAction animation;
    uint8_t animation_activation;
    uint64_t animation_started;
    float animation_phase, animation_blend;
    uint8_t armor_visible;
    bool retained, visible, proxy;
} Placeholder;

typedef struct {
    uint64_t deadline;
    unsigned gear;
    const char *animation;
    uint8_t animation_activation, dye;
    SwmpGearPose pose;
    bool retained, visible, has_pose;
} GearActor;

// Owned by the guest menu update thread. The request address must stay fixed while the game owns it.
static Placeholder slots[SWMP_MAX_PEERS];
static GearActor gear_slots[SWMP_VISUAL_SLOTS][SWMP_MAX_PEERS];
static uint64_t storage, gear_storage[SWMP_VISUAL_SLOTS], lobby_generation;
static uint64_t local_context, local_actor;
static uint32_t local_area;
static uint64_t applied_clock_at;
static int16_t local_yaw;
static uint64_t local_motion_at;
static float local_motion_pos[3];
static SwmpProxyAction local_motion_state, local_motion_action;
static int16_t local_motion_yaw, local_glide_roll;
static uint64_t local_shield_at;
static bool local_shield_break;
static uint8_t local_jump_activation;
static bool local_jumping;
static float local_jump_frame;
static uint64_t local_armor_at;
static struct {
    uint32_t model[SWMP_ARMOR_SLOTS];
    uint8_t dye[SWMP_ARMOR_SLOTS];
} local_armor;

RT_ON_RESET(multiplayer_presence_reset) {
    storage = lobby_generation = 0;
    local_context = local_actor = 0;
    local_area = 0;
    applied_clock_at = 0;
    local_yaw = 0;
    local_motion_at = 0;
    local_shield_at = 0;
    local_shield_break = false;
    local_jump_activation = 0;
    local_jumping = false;
    local_armor_at = 0;
    memset(&local_armor, 0, sizeof local_armor);
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

static const SwmpArmorAsset *find_armor(SwmpArmorSlot part, uint32_t id) {
    const SwmpArmorCatalog *catalog = fs_multiplayer_armor_catalog();
    if (!catalog || !id) return NULL;
    for (unsigned i = 0; i < catalog->model_count; i++) {
        const SwmpArmorAsset *asset = &catalog->models[i];
        if (asset->id == id && asset->slot == part && asset->ready) return asset;
    }
    return NULL;
}

static void retire_gear(Cpu *cpu, uint8_t peer, unsigned kind, uint64_t retry_at) {
    GearActor *slot = &gear_slots[kind][peer];
    release_actor(cpu, gear_storage[kind] + peer * SLOT_SIZE, slot->retained);
    const SwmpArmorAsset *armor = kind >= SWMP_FIRST_ARMOR_SLOT ? find_armor(kind - SWMP_FIRST_ARMOR_SLOT, slot->gear) : NULL;
    const char *label = kind < SWMP_FIRST_ARMOR_SLOT ? swmp_gear_assets[slot->gear].label : armor ? armor->model : "armor";
    LOG("[multiplayer] remote %s %s for peer %u\n", label,
        slot->visible ? "removed" : "request retired", peer);
    *slot = (GearActor){.deadline = retry_at};
}

static void retire(Cpu *cpu, uint8_t peer, uint64_t now, const char *reason) {
    Placeholder *slot = &slots[peer];
    for (unsigned kind = 0; kind < SWMP_VISUAL_SLOTS; kind++)
        if (gear_slots[kind][peer].gear) retire_gear(cpu, peer, kind, now + retry_ns);
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

static const char *animation_state(Cpu *cpu, uint64_t controller, unsigned layer, unsigned slot) {
    uint64_t string = call(cpu, g_site.mp_anim_current, controller, layer, slot);
    if (!guest_range(string, 16)) return "";
    uint64_t name = LD64(string + 8);
    if (!guest_range(name, 64)) return "";
    const char *state = (const char *)(uintptr_t)name;
    return memchr(state, 0, 64) ? state : "";
}

static const char *const bow_animation_names[SWMP_BOW_ANIM_COUNT] = {
    [SWMP_BOW_ANIM_HOLD] = "Hold",
    [SWMP_BOW_ANIM_EQUIPPED] = "Equiped",
    [SWMP_BOW_ANIM_DRAW] = "Draw",
    [SWMP_BOW_ANIM_SHOOT] = "Shoot",
    [SWMP_BOW_ANIM_NEUTRAL] = "Null",
    [SWMP_BOW_ANIM_CAMERA_PAUSE] = "SelfCameraPause",
};

static SwmpBowAnimation sample_bow_animation(Cpu *cpu, uint64_t bow, uint64_t heap_size) {
    uint64_t controller = g_site.mp_anim_current ? animation_controller(bow, heap_size) : 0;
    if (!controller) return SWMP_BOW_ANIM_NONE;
    const char *state = animation_state(cpu, controller, 0, 0);
    for (unsigned i = SWMP_BOW_ANIM_HOLD; i < SWMP_BOW_ANIM_COUNT; i++)
        if (!strcmp(state, bow_animation_names[i])) return i;
    return SWMP_BOW_ANIM_NONE;
}

static bool animation_frame(uint64_t controller, uint64_t heap_size, float *frame, float *duration) {
    uint64_t layer = LD64(controller + 0x08);
    if (!heap_object(layer, heap_size, 0xd8)) return false;
    uint64_t slot = LD64(layer + 0xd0);
    if (!heap_object(slot, heap_size, 0x50)) return false;
    uint64_t track = LD64(slot + 0x48);
    if (!heap_object(track, heap_size, 0x34)) return false;
    memcpy(frame, (const void *)(uintptr_t)(track + 0x28), sizeof *frame);
    memcpy(duration, (const void *)(uintptr_t)(track + 0x30), sizeof *duration);
    return *duration > 0 && *duration < 1600 && *frame >= 0 && *frame <= *duration;
}

static void sample_jump_phase(uint64_t controller, uint64_t heap_size, SwmpPlayerSnapshot *sample) {
    // The 1.9.0 standing Jump uses a single ten-frame track in the base slot.
    float frame, duration;
    if (!animation_frame(controller, heap_size, &frame, &duration) || duration != 10.0f) frame = -1.0f;
    if (!local_jumping || (frame >= 0 && frame + 0.5f < local_jump_frame))
        if (!++local_jump_activation) ++local_jump_activation;
    if (frame >= 0 || !local_jumping) local_jump_frame = frame;
    local_jumping = true;
    sample->animation_activation = local_jump_activation;
    if (frame >= 0) sample->animation_phase = 1 + (uint8_t)lroundf(frame * 25.4f);
}

static SwmpProxyAction sample_action(Cpu *cpu, uint64_t actor, uint64_t heap_size, uint64_t now, SwmpGearSet gear,
                                     const SwmpGearPose *weapon_pose, const SwmpGearPose *bow_pose,
                                     bool shield_raised, SwmpPlayerSnapshot *sample) {
    uint64_t controller = g_site.mp_anim_current ? animation_controller(actor, heap_size) : 0;
    if (!controller) {
        local_shield_at = 0;
        local_shield_break = false;
        local_jumping = false;
        return SWMP_PROXY_IDLE;
    }
    const char *state = animation_state(cpu, controller, 0, 0);
    if (!strcmp(state, "Jump") && !strcmp(g_game_version, "1.9.0"))
        sample_jump_phase(controller, heap_size, sample);
    else
        local_jumping = false;
    bool has_shield = gear.id[SWMP_GEAR_SHIELD] != SWMP_GEAR_NONE;
    // The native shield can disappear one update before the controller enters GuardBreak.
    bool recent_shield = local_shield_at && now - local_shield_at <= 2 * snapshot_ns;
    local_shield_break = !has_shield && (local_shield_break || recent_shield) && !strcmp(state, "GuardBreak");
    if (has_shield) local_shield_at = now;
    if (local_shield_break) return SWMP_PROXY_SHIELD_BREAK;
    // Standing bow actions play in the upper-body slot over the native WaitAttention state.
    const char *upper = gear.id[SWMP_GEAR_BOW] ? animation_state(cpu, controller, 1, 1) : "";
    return swmp_proxy_sample_action(state, upper, gear, weapon_pose, bow_pose, shield_raised);
}

static float motion_right(float dx, float dz) {
    // The actor's local X axis points to Link's left.
    float yaw = local_yaw * (pi / 32768.0f);
    return sinf(yaw) * dz - cosf(yaw) * dx;
}

static unsigned climb_direction(float side, float up) {
    return (lroundf(atan2f(up, side) * (4 / pi)) + 8) % 8;
}

static SwmpProxyAction sample_climb_jump(const float pos[3], uint64_t now) {
    static const SwmpProxyAction directions[] = {
        SWMP_PROXY_CLIMB_JUMP_RIGHT, SWMP_PROXY_CLIMB_JUMP_UP_RIGHT, SWMP_PROXY_CLIMB_JUMP,
        SWMP_PROXY_CLIMB_JUMP_UP_LEFT, SWMP_PROXY_CLIMB_JUMP_LEFT,
    };
    if (!local_motion_at || local_motion_state != SWMP_PROXY_CLIMB_JUMP) {
        local_motion_at = now;
        memcpy(local_motion_pos, pos, sizeof local_motion_pos);
        local_motion_state = SWMP_PROXY_CLIMB_JUMP;
        local_motion_action = SWMP_PROXY_CLIMB_WAIT;
    } else if (local_motion_action == SWMP_PROXY_CLIMB_WAIT) {
        float right = motion_right(pos[0] - local_motion_pos[0], pos[2] - local_motion_pos[2]);
        float up = fmaxf(pos[1] - local_motion_pos[1], 0);
        float distance = hypotf(right, up);
        uint64_t elapsed = now - local_motion_at;
        // Wait for launch motion before starting the one-shot clip, then hold it until the native state changes.
        if (distance >= 0.06f && distance < 1.0f && elapsed <= UINT64_C(300000000))
            local_motion_action = directions[climb_direction(right, up)];
        else if (elapsed >= UINT64_C(100000000))
            local_motion_action = SWMP_PROXY_CLIMB_JUMP;
    }
    return local_motion_action;
}

static SwmpProxyAction sample_step(const float pos[3], uint64_t now, SwmpProxyAction state) {
    static const SwmpProxyAction landings[] = {
        [0] = SWMP_PROXY_STEP_LAND,
        [SWMP_PROXY_STEP_LEFT - SWMP_PROXY_STEP] = SWMP_PROXY_STEP_LAND_LEFT,
        [SWMP_PROXY_STEP_RIGHT - SWMP_PROXY_STEP] = SWMP_PROXY_STEP_LAND_RIGHT,
        [SWMP_PROXY_STEP_FORWARD - SWMP_PROXY_STEP] = SWMP_PROXY_STEP_LAND_FORWARD,
        [SWMP_PROXY_STEP_BACK - SWMP_PROXY_STEP] = SWMP_PROXY_STEP_LAND_BACK,
    };
    if (state == SWMP_PROXY_STEP_LAND) {
        if (!local_motion_at || (local_motion_state != SWMP_PROXY_STEP && local_motion_state != SWMP_PROXY_STEP_LAND) ||
            local_motion_action == SWMP_PROXY_IDLE)
            local_motion_action = SWMP_PROXY_STEP;
        local_motion_state = SWMP_PROXY_STEP_LAND;
        return landings[local_motion_action - SWMP_PROXY_STEP];
    }
    if (!local_motion_at || local_motion_state != SWMP_PROXY_STEP) {
        local_motion_at = now;
        memcpy(local_motion_pos, pos, sizeof local_motion_pos);
        local_motion_yaw = local_yaw;
        local_motion_state = SWMP_PROXY_STEP;
        local_motion_action = SWMP_PROXY_IDLE;
    } else if (local_motion_action == SWMP_PROXY_IDLE) {
        float dx = pos[0] - local_motion_pos[0], dz = pos[2] - local_motion_pos[2];
        float distance = hypotf(dx, dz);
        uint64_t elapsed = now - local_motion_at;
        // A moving step covers about 18 cm in one 30 FPS update. Wait for launch motion,
        // then hold the direction through landing even when Link stops moving.
        if (distance >= 0.06f && distance < 1.0f && elapsed <= 2 * snapshot_ns) {
            float yaw = local_motion_yaw * (pi / 32768.0f);
            float forward = sinf(yaw) * dx + cosf(yaw) * dz;
            float right = sinf(yaw) * dz - cosf(yaw) * dx;
            if (fabsf(right) > fabsf(forward))
                local_motion_action = right > 0 ? SWMP_PROXY_STEP_RIGHT : SWMP_PROXY_STEP_LEFT;
            else
                local_motion_action = forward > 0 ? SWMP_PROXY_STEP_FORWARD : SWMP_PROXY_STEP_BACK;
        } else if (elapsed >= snapshot_ns) {
            local_motion_action = SWMP_PROXY_STEP;
        }
    }
    return local_motion_action;
}

static float motion_turn_rate(float seconds) {
    return remainderf((local_yaw - local_motion_yaw) * (pi / 32768.0f), 2 * pi) / seconds;
}

static SwmpProxyAction swim_motion(float speed, float turn) {
    SwmpProxyAction previous = local_motion_action;
    bool fast = previous == SWMP_PROXY_SWIM_FAST || previous == SWMP_PROXY_SWIM_FAST_LEFT ||
        previous == SWMP_PROXY_SWIM_FAST_RIGHT;
    bool left = previous == SWMP_PROXY_SWIM_SLOW_LEFT || previous == SWMP_PROXY_SWIM_FAST_LEFT;
    bool right = previous == SWMP_PROXY_SWIM_SLOW_RIGHT || previous == SWMP_PROXY_SWIM_FAST_RIGHT;
    // Full forward strokes reach about 1.9 m/s. Positive yaw turns to Link's left.
    fast = speed >= (fast ? 1.15f : 1.30f);
    if (turn >= (left ? 16 : 30) * (pi / 180))
        return fast ? SWMP_PROXY_SWIM_FAST_LEFT : SWMP_PROXY_SWIM_SLOW_LEFT;
    if (turn <= -(right ? 16 : 30) * (pi / 180))
        return fast ? SWMP_PROXY_SWIM_FAST_RIGHT : SWMP_PROXY_SWIM_SLOW_RIGHT;
    return fast ? SWMP_PROXY_SWIM_FAST : SWMP_PROXY_SWIM_MOVE;
}

static SwmpProxyAction swim_lock_direction(float dx, float dz) {
    float yaw = local_yaw * (pi / 32768.0f);
    float forward = sinf(yaw) * dx + cosf(yaw) * dz;
    float right = motion_right(dx, dz);
    bool sideways = local_motion_action == SWMP_PROXY_SWIM_LOCK_LEFT ||
        local_motion_action == SWMP_PROXY_SWIM_LOCK_RIGHT;
    // Keep the previous axis near a diagonal, while still selecting the current sign.
    if (fabsf(right) > fabsf(forward) * (sideways ? 0.8f : 1.25f))
        return right > 0 ? SWMP_PROXY_SWIM_LOCK_RIGHT : SWMP_PROXY_SWIM_LOCK_LEFT;
    return forward >= 0 ? SWMP_PROXY_SWIM_LOCK_FORWARD : SWMP_PROXY_SWIM_LOCK_BACK;
}

static SwmpProxyAction sample_motion(const float pos[3], uint64_t now, SwmpProxyAction state) {
    static const SwmpProxyAction directions[] = {
        SWMP_PROXY_CLIMB_RIGHT, SWMP_PROXY_CLIMB_UP_RIGHT, SWMP_PROXY_CLIMB_MOVE, SWMP_PROXY_CLIMB_UP_LEFT,
        SWMP_PROXY_CLIMB_LEFT, SWMP_PROXY_CLIMB_DOWN_LEFT, SWMP_PROXY_CLIMB_DOWN, SWMP_PROXY_CLIMB_DOWN_RIGHT,
    };
    if (state != local_motion_state) local_motion_at = 0;
    uint64_t elapsed = now - local_motion_at;
    if (local_motion_at && elapsed < UINT64_C(100000000)) return local_motion_action;

    SwmpProxyAction action = state == SWMP_PROXY_CLIMB_MOVE ? SWMP_PROXY_CLIMB_WAIT : state;
    if (state == SWMP_PROXY_SWIM_LOCK_FORWARD) action = SWMP_PROXY_SWIM_WAIT;
    if (local_motion_at && elapsed <= UINT64_C(300000000)) {
        float dx = pos[0] - local_motion_pos[0], dz = pos[2] - local_motion_pos[2];
        float seconds = elapsed * 1e-9f;
        float speed = hypotf(dx, dz) / seconds;
        if (state == SWMP_PROXY_CLIMB_MOVE) {
            float right = motion_right(dx, dz);
            float up = pos[1] - local_motion_pos[1];
            float climb_speed = hypotf(right, up) / seconds;
            if (climb_speed >= 0.1f && climb_speed < 10.0f && speed < 10.0f)
                action = directions[climb_direction(right, up)];
        } else if (state == SWMP_PROXY_SWIM_MOVE) {
            if (speed < 4.0f) action = swim_motion(speed, motion_turn_rate(seconds));
        } else if (state == SWMP_PROXY_SWIM_LOCK_FORWARD) {
            if (speed >= 0.1f && speed < 4.0f) action = swim_lock_direction(dx, dz);
        } else if (speed < 10.0f) {
            // The native Move state blends walking, running and sprinting. Lower exit thresholds avoid flicker.
            if (speed >= (local_motion_action == SWMP_PROXY_DASH ? 5.7f : 6.2f)) action = SWMP_PROXY_DASH;
            else if (speed >= (local_motion_action == SWMP_PROXY_WALK ? 3.4f : 2.9f)) action = SWMP_PROXY_RUN;
        }
    }
    local_motion_at = now;
    memcpy(local_motion_pos, pos, sizeof local_motion_pos);
    local_motion_state = state;
    local_motion_yaw = local_yaw;
    return local_motion_action = action;
}

static bool sample_facing(uint64_t actor, uint64_t heap_size, const float pos[3]) {
    if (!heap_object(actor, heap_size, 0x3c8)) return false;
    float matrix[12];
    memcpy(matrix, (const void *)(uintptr_t)(actor + 0x398), sizeof matrix);
    // Confirm the published actor transform belongs to the current player placement.
    for (unsigned i = 0; i < 3; i++)
        if (!(fabsf(matrix[3 + 4 * i] - pos[i]) < 2.0f)) return false;
    float x = matrix[2], z = matrix[10], length_sq = x * x + z * z;
    if (!(length_sq > 0.5f && length_sq < 1.5f)) return false;
    long turn = lround(atan2((double)x, (double)z) * (32768.0 / pi));
    local_yaw = (int16_t)(turn == 32768 ? -32768 : turn);
    return true;
}

static int16_t quantize_turn(float angle) {
    long turn = lroundf(remainderf(angle, 2 * pi) * (32768.0f / pi));
    return (int16_t)(turn == 32768 ? -32768 : turn);
}

static float clamp_glide_bank(float roll) {
    return fminf(glide_bank_limit, fmaxf(-glide_bank_limit, roll));
}

static int16_t sample_glide_bank(const float pos[3], uint64_t now) {
    if (local_motion_state != SWMP_PROXY_GLIDE) local_motion_at = 0;
    uint64_t elapsed = now - local_motion_at;
    if (local_motion_at && elapsed < snapshot_ns) return local_glide_roll;

    float roll = 0;
    if (local_motion_at && elapsed <= UINT64_C(250000000) &&
        hypotf(pos[0] - local_motion_pos[0], pos[2] - local_motion_pos[2]) < 3.0f) {
        // The native root stays upright. Positive yaw turns left, so lean the left side down.
        roll = -0.7f * motion_turn_rate(elapsed * 1e-9f);
    }
    local_motion_at = now;
    local_motion_state = SWMP_PROXY_GLIDE;
    local_motion_yaw = local_yaw;
    memcpy(local_motion_pos, pos, sizeof local_motion_pos);
    return local_glide_roll = quantize_turn(clamp_glide_bank(roll));
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
                               SwmpPlayerSnapshot *sample, uint64_t *bow) {
    SwmpGearSet gear = {0};
    if (!g_site.player_equipment) return gear;
    // The equipment getter uses the same category ordering as the equipped-item drop path.
    static const uint8_t categories[SWMP_GEAR_SLOTS] = {
        [SWMP_GEAR_WEAPON] = 0, [SWMP_GEAR_SHIELD] = 1, [SWMP_GEAR_BOW] = 2,
    };
    for (unsigned slot = 0; slot < SWMP_GEAR_SLOTS; slot++) {
        uint64_t item = call(cpu, g_site.player_equipment, actor, categories[slot], 0);
        if (!heap_object(item, heap_size, 0x3c8) || LD8(item + 0x64) != 1 || (LD32(item + 0x78) & 1)) continue;
        uint64_t name = LD64(item + 0x10);
        for (unsigned id = 1; id < SWMP_GEAR_COUNT; id++) {
            const SwmpGearAsset *asset = &swmp_gear_assets[id];
            if (asset->slot != slot || !action_named(name, asset->model)) continue;
            gear.id[slot] = id;
            if (sample_gear_pose(item, pos, sample->rotation[1], &sample->gear_pose[slot])) {
                sample->flags |= swmp_gear_pose_flag(slot);
                if (slot == SWMP_GEAR_BOW) *bow = item;
            }
            break;
        }
    }
    return gear;
}

static void sample_armor(Cpu *cpu, uint64_t actor, uint64_t heap_size, uint64_t now, SwmpGearSet *gear) {
    const SwmpArmorCatalog *catalog = fs_multiplayer_armor_catalog();
    // These are the native retained head, upper and lower armor indices in 1.9.0.
    const uint64_t first_index = 0x23f0;
    if (!catalog || !g_site.actor_index_resolve || strcmp(g_game_version, "1.9.0") ||
        !heap_object(actor, heap_size, first_index + 0x10 * SWMP_ARMOR_SLOTS)) return;
    if (!local_armor_at || now - local_armor_at >= snapshot_ns) {
        Cpu scratch = *cpu;
        scratch.sp -= 0x40;
        uint64_t link = scratch.sp + 0x10;
        local_armor_at = now;
        for (unsigned part = 0; part < SWMP_ARMOR_SLOTS; part++) {
            uint64_t index = actor + first_index + 0x10 * part;
            memset((void *)(uintptr_t)link, 0, 0x18);
            uint64_t item = call(&scratch, g_site.actor_index_resolve, index, link, 0);
            if (heap_object(item, heap_size, 0x7c) && LD64(link + 8) == item && LD8(item + 0x64) == 1 &&
                !(LD32(item + 0x78) & 1)) {
                uint64_t name = LD64(item + 0x10);
                if (guest_range(name, 48) && memchr((const void *)(uintptr_t)name, 0, 48)) {
                    local_armor.model[part] = swmp_armor_identity(catalog, part, (const char *)(uintptr_t)name);
                    uint32_t dye = local_armor.model[part] && g_site.mp_armor_dye ?
                        (uint32_t)call(&scratch, g_site.mp_armor_dye, link, 0, 0) : 0;
                    local_armor.dye[part] = dye <= SWMP_ARMOR_DYE_MAX ? dye : 0;
                }
            }
            call(&scratch, g_site.actor_link_release, link, 0, 0);
        }
    }
    memcpy(gear->armor, local_armor.model, sizeof gear->armor);
    memcpy(gear->armor_dye, local_armor.dye, sizeof gear->armor_dye);
}

static bool bow_holds_arrow(SwmpProxyAction action) {
    return action == SWMP_PROXY_BOW_RELOAD || action == SWMP_PROXY_BOW_AIM;
}

static SwmpArrow sample_arrow(uint64_t bow, uint64_t heap_size, const float pos[3], SwmpPlayerSnapshot *sample) {
    // The bow's attached actor becomes a projectile on release. Sample it only while nocked.
    uint64_t arrow = LD64(bow + 0x88);
    if (!heap_object(arrow, heap_size, 0x3c8) || LD8(arrow + 0x64) != 1 || (LD32(arrow + 0x78) & 1))
        return SWMP_ARROW_NONE;
    uint64_t name = LD64(arrow + 0x10);
    for (unsigned i = SWMP_ARROW_NORMAL; i < SWMP_ARROW_COUNT; i++) {
        const SwmpGearAsset *asset = &swmp_gear_assets[SWMP_ARROW_ASSET_BASE + i];
        if (action_named(name, asset->model_user))
            return sample_gear_pose(arrow, pos, sample->rotation[1], &sample->arrow_pose) ? i : SWMP_ARROW_NONE;
    }
    return SWMP_ARROW_NONE;
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
        local_shield_at = 0;
        local_shield_break = false;
        local_jumping = false;
        local_armor_at = 0;
        memset(&local_armor, 0, sizeof local_armor);
    }
    for (unsigned i = 0; i < 3; i++) sample.position_mm[i] = (int32_t)lround((double)p[i] * 1000);
    bool has_facing = sample_facing(actor, heap_size, p);
    sample.rotation[1] = local_yaw;
    uint64_t bow = 0;
    *gear = sample_gear(cpu, actor, heap_size, p, &sample, &bow);
    sample_armor(cpu, actor, heap_size, now, gear);
    const SwmpGearPose *weapon_pose = (sample.flags & SWMP_PLAYER_WEAPON_POSE) ?
        &sample.gear_pose[SWMP_GEAR_WEAPON] : NULL;
    // WaitAttention also covers lock-on. A raised shield is in front of Link in his yaw frame.
    bool shield_raised = (sample.flags & SWMP_PLAYER_SHIELD_POSE) &&
        sample.gear_pose[SWMP_GEAR_SHIELD].offset_mm[2] >= 175;
    const SwmpGearPose *bow_pose = (sample.flags & SWMP_PLAYER_BOW_POSE) ? &sample.gear_pose[SWMP_GEAR_BOW] : NULL;
    sample.action = sample_action(cpu, actor, heap_size, now, *gear, weapon_pose, bow_pose, shield_raised, &sample);
    // The bow starts drawing and finishes releasing independently of Link's upper-body state.
    if (bow) sample.bow_animation = sample_bow_animation(cpu, bow, heap_size);
    if (bow && bow_holds_arrow(sample.action) && !strcmp(g_game_version, "1.9.0"))
        sample.nocked_arrow = sample_arrow(bow, heap_size, p, &sample);
    if (sample.action == SWMP_PROXY_CROUCH_IDLE || sample.action == SWMP_PROXY_CROUCH_MOVE)
        sample.flags |= SWMP_PLAYER_CROUCHING;
    if (sample.action == SWMP_PROXY_SHIELD_GUARD) sample.flags |= SWMP_PLAYER_GUARDING;
    if (sample.action == SWMP_PROXY_GLIDE) {
        sample.flags |= SWMP_PLAYER_GLIDING;
        if (has_facing) sample.rotation[2] = sample_glide_bank(p, now);
        else local_motion_at = 0;
    } else if (sample.action == SWMP_PROXY_CLIMB_JUMP)
        sample.action = sample_climb_jump(p, now);
    else if (sample.action == SWMP_PROXY_STEP || sample.action == SWMP_PROXY_STEP_LAND)
        sample.action = sample_step(p, now, sample.action);
    else if (sample.action == SWMP_PROXY_WALK || sample.action == SWMP_PROXY_CLIMB_MOVE ||
             (has_facing && (sample.action == SWMP_PROXY_SWIM_MOVE || sample.action == SWMP_PROXY_SWIM_LOCK_FORWARD)))
        sample.action = sample_motion(p, now, sample.action);
    else {
        local_motion_at = 0;
        if (sample.action == SWMP_PROXY_SWIM_LOCK_FORWARD) sample.action = SWMP_PROXY_SWIM_WAIT;
    }
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
    sample.sampled_ms = (uint32_t)(now / UINT64_C(1000000));
    return sample;
}

static Pose snapshot_pose(const SwmpPlayerSnapshot *sample) {
    Pose pose = {0};
    for (unsigned i = 0; i < 3; i++) pose.position[i] = sample->position_mm[i] / 1000.0f;
    pose.yaw = sample->rotation[1] * (pi / 32768.0f);
    pose.action = sample->action < SWMP_PROXY_ACTION_COUNT ? sample->action : SWMP_PROXY_IDLE;
    if (pose.action == SWMP_PROXY_GLIDE && (sample->flags & SWMP_PLAYER_GLIDING))
        pose.roll = clamp_glide_bank(sample->rotation[2] * (pi / 32768.0f));
    return pose;
}

static void queue_playback(Placeholder *slot, const SwmpPosition *position, uint64_t now_ms) {
    unsigned count = slot->playback_count;
    PlaybackSample *samples = slot->playback;
    uint64_t play_at = position->received_at + playback_delay_ms;
    if (count) {
        PlaybackSample *last = &samples[count - 1];
        // A host batch can relay the same source sample several times. Keep its original time.
        uint32_t elapsed = position->snapshot.sampled_ms - last->snapshot.sampled_ms;
        if (!elapsed || elapsed > UINT32_MAX / 2) return;
        if (elapsed >= SWMP_POSITION_TIMEOUT_MS) {
            count = 0;
        } else {
            play_at = last->play_at + elapsed;
            uint64_t latest = position->received_at + playback_delay_ms + SWMP_POSITION_INTERVAL_MS;
            if (play_at > latest) play_at = latest;
            // After an underrun, resume from the position we held. Playing the late
            // sample immediately would advance by several frames in a single update.
            if (count == 1 && last->play_at < now_ms) {
                last->play_at = now_ms;
                if (play_at < now_ms + SWMP_POSITION_INTERVAL_MS)
                    play_at = now_ms + SWMP_POSITION_INTERVAL_MS;
            }
            if (play_at <= last->play_at) play_at = last->play_at + 1;
        }
    }
    if (count == PLAYBACK_SAMPLES) {
        count--;
        memmove(samples, samples + 1, count * sizeof *samples);
    }
    samples[count] = (PlaybackSample){.snapshot = position->snapshot, .play_at = play_at};
    slot->playback_count = count + 1;
}

static SwmpGearPose interpolate_gear(const SwmpGearPose *a, const SwmpGearPose *b, float t) {
    SwmpGearPose pose;
    for (unsigned i = 0; i < 3; i++) {
        pose.offset_mm[i] = (int16_t)lroundf(a->offset_mm[i] + t * (b->offset_mm[i] - a->offset_mm[i]));
        float turn = remainderf(b->rotation[i] - a->rotation[i], 65536.0f);
        pose.rotation[i] = (int16_t)lroundf(a->rotation[i] + t * turn);
    }
    return pose;
}

static SwmpPlayerSnapshot sample_playback(Placeholder *slot, uint64_t now_ms, Pose *pose) {
    PlaybackSample *samples = slot->playback;
    unsigned first = 0;
    while (first + 1 < slot->playback_count && samples[first + 1].play_at <= now_ms) first++;
    if (first) {
        slot->playback_count -= first;
        memmove(samples, samples + first, slot->playback_count * sizeof *samples);
    }
    const SwmpPlayerSnapshot *a = &samples[0].snapshot, *b = &samples[1].snapshot;
    SwmpPlayerSnapshot result = *a;
    *pose = snapshot_pose(a);
    if (slot->playback_count == 1 || now_ms <= samples[0].play_at) return result;

    Pose target = snapshot_pose(b);
    float distance_sq = 0;
    for (unsigned i = 0; i < 3; i++) {
        float d = target.position[i] - pose->position[i];
        distance_sq += d * d;
    }
    // A teleport is shown at its snapshot boundary.
    if (distance_sq > 9 || a->phase_id != b->phase_id) return result;

    float t = (float)(now_ms - samples[0].play_at) / (float)(samples[1].play_at - samples[0].play_at);
    for (unsigned i = 0; i < 3; i++) pose->position[i] += t * (target.position[i] - pose->position[i]);
    pose->yaw = remainderf(pose->yaw + t * remainderf(target.yaw - pose->yaw, 2 * pi), 2 * pi);
    pose->roll += t * (target.roll - pose->roll);
    if (a->equipment_revision == b->equipment_revision) {
        for (unsigned i = 0; i < SWMP_GEAR_SLOTS; i++)
            if (a->flags & b->flags & swmp_gear_pose_flag(i))
                result.gear_pose[i] = interpolate_gear(&a->gear_pose[i], &b->gear_pose[i], t);
        if (a->nocked_arrow && a->nocked_arrow == b->nocked_arrow)
            result.arrow_pose = interpolate_gear(&a->arrow_pose, &b->arrow_pose, t);
    }
    if (a->action == b->action && a->animation_activation == b->animation_activation &&
        a->animation_phase && b->animation_phase)
        result.animation_phase = (uint8_t)lroundf(a->animation_phase + t * (b->animation_phase - a->animation_phase));
    return result;
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

static void copy_transform(uint64_t dst, const Pose *pose, float lift) {
    float s = sinf(pose->yaw), c = cosf(pose->yaw);
    float sr = sinf(pose->roll), cr = cosf(pose->roll);
    const float *pos = pose->position;
    // Lift along the tilted local Y axis so the canopy stays above Link's hands in a turn.
    float matrix[12] = {
        c*cr, -c*sr, s, pos[0] - lift*c*sr,
        sr, cr, 0, pos[1] + lift*cr,
        -s*cr, s*sr, c, pos[2] + lift*s*sr,
    };
    memcpy((void *)(uintptr_t)dst, matrix, sizeof matrix);
}

// The placement request copies a 0xc4-byte parameter list from the argument at +0x10.
enum { SPAWN_PARAM_OFFSET = 0x10, SPAWN_PARAM_BYTES = 0xc4, SPAWN_DYE_BYTES = 13 };

static bool request_actor(Cpu *source, uint64_t addr, uint64_t context, const Pose *pose, const char *actor_name,
                          uint8_t dye) {
    Cpu cpu = *source;
    uint64_t args = 0;
    if (dye && g_site.mp_armor_dye_key) {
        cpu.sp -= 0xe0;
        args = cpu.sp;
        memset((void *)(uintptr_t)args, 0, SPAWN_PARAM_OFFSET + SPAWN_PARAM_BYTES);
        uint64_t params = args + SPAWN_PARAM_OFFSET;
        // One signed integer entry. The name points to a resident native placement key.
        ST16(params, 1);
        ST16(params + 2, SPAWN_DYE_BYTES);
        ST64(params + 4, g_site.mp_armor_dye_key);
        ST8(params + 12, 0);
        ST32(params + 13, dye);
    }
    cpu.sp -= 0x80;
    uint64_t name = cpu.sp + 0x10, transform = cpu.sp + 0x30;
    memcpy((void *)(uintptr_t)name, actor_name, strlen(actor_name) + 1);
    copy_transform(transform, pose, 0);
    cpu.x[0] = name;
    cpu.x[1] = transform;
    cpu.x[2] = context;
    cpu.x[3] = addr;
    cpu.x[4] = UINT32_MAX;  // no life override or placement ID
    cpu.x[5] = args;
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

static void move_actor(Cpu *source, uint64_t actor, const Pose *pose, float lift) {
    Cpu cpu = *source;
    cpu.sp -= 0x40;
    copy_transform(cpu.sp + 0x10, pose, lift);
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

static bool play_animation(Cpu *source, uint64_t actor, const char *state, float blend_frames, float phase);

static uint64_t appearance_model(uint64_t actor, uint64_t heap_size) {
    if (!heap_object(actor, heap_size, 0x4e8)) return 0;
    uint64_t model = LD64(actor + 0x4e0);
    return heap_object(model, heap_size, 0xe0) ? model : 0;
}

static void set_armor_visibility(Cpu *source, uint64_t body, uint64_t cover, bool visible) {
    Cpu cpu = *source;
    cpu.x[0] = 0;
    cpu.x[1] = body;
    cpu.x[2] = cover;
    cpu.x[3] = visible;
    guest_call(&cpu, g_site.mp_armor_visibility);
}

static void update_armor_visibility(Cpu *cpu, uint8_t peer) {
    Placeholder *link = &slots[peer];
    if (!g_site.mp_armor_visibility || !link->visible || !link->proxy) return;
    uint8_t visible = 0;
    for (unsigned part = 0; part < SWMP_ARMOR_SLOTS; part++)
        if (gear_slots[SWMP_FIRST_ARMOR_SLOT + part][peer].visible) visible |= 1u << part;
    if (visible == link->armor_visible) return;

    uint64_t heap_size = kernel_heap_size();
    uint64_t addr = storage + peer * SLOT_SIZE;
    uint64_t body = appearance_model(resolve(cpu, addr), heap_size);
    if (body) {
        uint8_t applied = 0;
        set_armor_visibility(cpu, body, 0, true);
        // Native Link resets the grouped meshes, then lets each active piece hide its covered groups.
        for (unsigned part = 0; part < SWMP_ARMOR_SLOTS; part++) {
            if (!(visible & (1u << part))) continue;
            uint64_t armor_addr = gear_storage[SWMP_FIRST_ARMOR_SLOT + part] + peer * SLOT_SIZE;
            uint64_t cover = appearance_model(resolve(cpu, armor_addr), heap_size);
            if (cover) {
                set_armor_visibility(cpu, body, cover, false);
                applied |= 1u << part;
            }
            call(cpu, g_site.actor_link_release, armor_addr + LINK_OFFSET, 0, 0);
        }
        link->armor_visible = applied;
    }
    call(cpu, g_site.actor_link_release, addr + LINK_OFFSET, 0, 0);
}

static void update_visual(Cpu *cpu, uint8_t peer, unsigned kind, uint64_t now, uint64_t context,
                          unsigned id, const SwmpGearPose *pose, const char *animation, uint8_t dye) {
    const Placeholder *link = &slots[peer];
    GearActor *slot = &gear_slots[kind][peer];
    bool skinned = kind >= SWMP_FIRST_ARMOR_SLOT;
    if (slot->gear && (slot->gear != id || (skinned && slot->dye != dye))) {
        // Arrow release and replacement are routine. Leave a send interval for deletion
        // without imposing the one-second failure backoff on the next nock.
        retire_gear(cpu, peer, kind, now + (kind == SWMP_NOCKED_ARROW_SLOT || skinned ? snapshot_ns : retry_ns));
        return;
    }
    if (!link->visible || !link->proxy || !id) return;
    const SwmpArmorAsset *armor = skinned ? find_armor(kind - SWMP_FIRST_ARMOR_SLOT, id) : NULL;
    if (skinned ? !armor : !fs_multiplayer_gear_ready(id)) return;
    // A delayed equipment record or a missing native pose should leave the last
    // good relative offset attached to Link while the same item is still equipped.
    if (pose) {
        slot->pose = *pose;
        slot->has_pose = true;
    }
    if (!skinned && !(slot->gear && slot->has_pose) && !pose && id != SWMP_PARAGLIDER_ASSET) return;
    if (!gear_storage[kind]) {
        gear_storage[kind] = actor_slots_alloc();
        if (!gear_storage[kind]) return;
    }
    uint64_t addr = gear_storage[kind] + peer * SLOT_SIZE;
    if (!slot->gear) {
        if (now < slot->deadline) return;
        slot->gear = id;
        slot->dye = dye;
        slot->deadline = now + request_ns;
        const char *actor_name = armor ? armor->actor : swmp_gear_assets[id].actor;
        if (!request_actor(cpu, addr, context, &link->applied, actor_name, dye))
            retire_gear(cpu, peer, kind, now + retry_ns);
    } else if (slot->retained) {
        uint64_t actor = resolve(cpu, addr);
        bool active = actor && LD8(actor + 0x64) == 1 && !(LD32(actor + 0x78) & 1);
        if (active) {
            if (skinned)
                move_actor(cpu, actor, &link->applied, link->model_lift);
            else if (id == SWMP_PARAGLIDER_ASSET)
                move_actor(cpu, actor, &link->applied, 2.0f);
            else
                move_gear(cpu, actor, &link->applied, &slot->pose);
            uint8_t activation = skinned ? link->animation_activation : 0;
            if (animation && (slot->animation != animation || slot->animation_activation != activation)) {
                float blend = skinned && slot->animation && link->animation_started == now ? link->animation_blend : -1.0f;
                float phase = skinned ? link->animation_phase : -1.0f;
                if (play_animation(cpu, actor, animation, blend, phase)) {
                    slot->animation = animation;
                    slot->animation_activation = activation;
                }
            }
            if (!slot->visible)
                LOG("[multiplayer] remote %s ready for peer %u\n", armor ? armor->model : swmp_gear_assets[id].label, peer);
            slot->visible = true;
        }
        call(cpu, g_site.actor_link_release, addr + LINK_OFFSET, 0, 0);
        if (!active && (slot->visible || now >= slot->deadline)) retire_gear(cpu, peer, kind, now + retry_ns);
    } else if (call(cpu, g_site.mp_presence_ready, addr, 0, 0) & 1) {
        slot->retained = retain_actor(cpu, addr);
        if (!slot->retained) retire_gear(cpu, peer, kind, now + retry_ns);
    } else if (now >= slot->deadline || (call(cpu, g_site.mp_presence_failed, addr, 0, 0) & 1)) {
        retire_gear(cpu, peer, kind, now + retry_ns);
    }
}

static void update_gear(Cpu *cpu, uint8_t peer, uint64_t now, uint64_t context,
                        const SwmpPlayerSnapshot *sample, const SwmpEquipment *gear, SwmpProxyAction action) {
    bool matching_revision = gear->revision == sample->equipment_revision;
    for (unsigned kind = 0; kind < SWMP_GEAR_SLOTS; kind++) {
        // The reliable update can arrive before or after its pose reaches playback.
        unsigned id = matching_revision ? gear->gear.id[kind] : gear_slots[kind][peer].gear;
        const SwmpGearPose *pose = NULL;
        const char *animation = NULL;
        if (matching_revision && (sample->flags & swmp_gear_pose_flag(kind))) {
            pose = &sample->gear_pose[kind];
            if (kind == SWMP_GEAR_BOW) animation = bow_animation_names[sample->bow_animation];
        }
        update_visual(cpu, peer, kind, now, context, id, pose, animation, 0);
    }
    bool deployed = action == SWMP_PROXY_GLIDE && (sample->flags & SWMP_PLAYER_GLIDING);
    update_visual(cpu, peer, SWMP_PARAGLIDER_SLOT, now, context,
                  deployed ? SWMP_PARAGLIDER_ASSET : SWMP_GEAR_NONE, NULL, "Open", 0);
    bool nocked = sample->nocked_arrow != SWMP_ARROW_NONE && bow_holds_arrow(action) &&
        matching_revision && gear_slots[SWMP_GEAR_BOW][peer].visible;
    update_visual(cpu, peer, SWMP_NOCKED_ARROW_SLOT, now, context,
                  nocked ? SWMP_ARROW_ASSET_BASE + sample->nocked_arrow : SWMP_GEAR_NONE,
                  nocked ? &sample->arrow_pose : NULL, NULL, 0);

    const SwmpArmorCatalog *catalog = fs_multiplayer_armor_catalog();
    if (!catalog) return;
    const Placeholder *link = &slots[peer];
    const char *animation = link->animation < SWMP_PROXY_ACTION_COUNT ? swmp_proxy_action_name(link->animation) : NULL;
    for (unsigned part = 0; part < SWMP_ARMOR_SLOTS; part++) {
        unsigned kind = SWMP_FIRST_ARMOR_SLOT + part;
        const GearActor *slot = &gear_slots[kind][peer];
        uint32_t id = slot->gear;
        uint8_t dye = slot->dye;
        if (matching_revision) {
            id = gear->gear.armor[part];
            dye = gear->gear.armor_dye[part];
            // Link's base model contains bare skin. The default head supplies his hair.
            if (part == SWMP_ARMOR_HEAD && !find_armor(part, id)) {
                id = catalog->defaults[part];
                dye = 0;
            }
        }
        update_visual(cpu, peer, kind, now, context, id, NULL, animation, dye);
    }
    update_armor_visibility(cpu, peer);
}

static bool play_animation(Cpu *source, uint64_t actor, const char *state, float blend_frames, float phase) {
    if (!g_site.mp_anim_play || !g_site.mp_anim_string) return false;
    uint64_t controller = animation_controller(actor, kernel_heap_size());
    if (!controller) return false;

    Cpu cpu = *source;
    cpu.sp -= 0x40;
    uint64_t name = cpu.sp + 0x20;
    memcpy((void *)(uintptr_t)name, state, strlen(state) + 1);
    ST64(cpu.sp, g_site.mp_anim_string + 0x10);  // SafeString vtable address point
    ST64(cpu.sp + 8, name);
    cpu.x[0] = controller;
    cpu.x[1] = cpu.sp;
    cpu.x[2] = cpu.x[3] = 0;  // layer and slot
    cpu.x[4] = 1;
    uint32_t bits[2];
    memcpy(&bits[0], &blend_frames, sizeof blend_frames);
    memcpy(&bits[1], &phase, sizeof phase);
    cpu.v[0] = bits[0];
    cpu.v[1] = bits[1];
    guest_call(&cpu, g_site.mp_anim_play);
    return true;
}

static bool same_pose(const Pose *a, const Pose *b) {
    return a->yaw == b->yaw && a->roll == b->roll && !memcmp(a->position, b->position, sizeof a->position);
}

static void update_proxy(Cpu *cpu, uint64_t actor, Placeholder *slot, const Pose *pose) {
    // The native swimmer places its model higher than the actor root. The standalone clips
    // need that lift to reach the waterline. Keep the sampled root for gear and range.
    bool swimming = pose->action == SWMP_PROXY_SWIM_WAIT || pose->action == SWMP_PROXY_SWIM_MOVE ||
        pose->action == SWMP_PROXY_SWIM_DASH ||
        (pose->action >= SWMP_PROXY_SWIM_FAST && pose->action <= SWMP_PROXY_SWIM_LOCK_RIGHT);
    float lift = swimming ? 1.65f : 0;
    if (!slot->visible || lift != slot->model_lift || !same_pose(&slot->applied, pose)) {
        slot->applied = *pose;
        slot->model_lift = lift;
        Pose next = *pose;
        next.position[1] += lift;
        move_actor(cpu, actor, &next, 0);
    }
}

static void poll_actor(Cpu *cpu, uint8_t peer, uint64_t now, const Pose *pose, const SwmpPlayerSnapshot *sample) {
    Placeholder *slot = &slots[peer];
    uint64_t addr = storage + peer * SLOT_SIZE;
    if (slot->retained) {
        uint64_t actor = resolve(cpu, addr);
        bool active = actor && LD8(actor + 0x64) == 1 && !(LD32(actor + 0x78) & 1);
        if (active && slot->proxy) {
            update_proxy(cpu, actor, slot, pose);
            uint8_t activation = pose->action == SWMP_PROXY_JUMP ? sample->animation_activation : 0;
            slot->animation_phase = -1.0f;
            if (slot->animation != pose->action || slot->animation_activation != activation) {
                SwmpProxyAction from = slot->animation, to = pose->action;
                // Standing Link's shield keeps moving for several updates around the guard state edge.
                bool raise = to == SWMP_PROXY_SHIELD_GUARD && (from == SWMP_PROXY_IDLE || from == SWMP_PROXY_SWORD_READY);
                bool lower = from == SWMP_PROXY_SHIELD_GUARD && (to == SWMP_PROXY_IDLE || to == SWMP_PROXY_SWORD_READY);
                float blend_frames = raise || lower ? 4.0f : -1.0f;
                float phase = activation && sample->animation_phase ? (sample->animation_phase - 1) / 254.0f : -1.0f;
                if (play_animation(cpu, actor, swmp_proxy_action_name(to), blend_frames, phase)) {
                    slot->animation = to;
                    slot->animation_activation = activation;
                    slot->animation_started = now;
                    slot->animation_phase = phase;
                    slot->animation_blend = blend_frames;
                }
            } else if (fs_multiplayer_armor_catalog()) {
                uint64_t heap_size = kernel_heap_size();
                uint64_t controller = animation_controller(actor, heap_size);
                float frame, duration;
                if (controller && animation_frame(controller, heap_size, &frame, &duration))
                    slot->animation_phase = frame / duration;
            }
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

// Called by the native variation loader while the placement parameters are still live.
uint32_t rt_mp_armor_spawn_dye(uint64_t actor, uint64_t args) {
#ifdef SW_MULTIPLAYER
    if (!g_site.mp_armor_dye_key || !guest_range(actor, 0x18) || !guest_range(args, 0x20)) return UINT32_MAX;
    uint64_t name = LD64(actor + 0x10);
    size_t prefix = sizeof SWMP_ARMOR_ACTOR_PREFIX - 1;
    if (!guest_range(name, prefix) ||
        memcmp((const void *)(uintptr_t)name, SWMP_ARMOR_ACTOR_PREFIX, prefix)) return UINT32_MAX;

    uint64_t ref = LD64(args + 0x18);
    uint64_t params = guest_range(ref, 8) ? LD64(ref) : 0;
    if (!guest_range(params, 4 + SPAWN_DYE_BYTES) || !LD16(params) || LD16(params + 2) < SPAWN_DYE_BYTES ||
        LD64(params + 4) != g_site.mp_armor_dye_key || LD8(params + 12) != 0) return UINT32_MAX;
    uint32_t dye = LD32(params + 13);
    if (dye <= SWMP_ARMOR_DYE_MAX) return dye;
#else
    (void)actor;
    (void)args;
#endif
    return UINT32_MAX;
}

void rt_mp_presence_tick(Cpu *cpu) {
#ifdef SW_MULTIPLAYER
    SwmpM1Roster roster = swmp_m1_roster();
    if (!roster.lobby_generation && !storage) return;
    if (!g_site.mp_presence_create || !g_site.mp_presence_move || !g_site.actor_index_assign ||
        (cpu->sp & 15) || !guest_range(cpu->sp - 0x80, 0xe0)) return;

    uint64_t now = plat_now_ns(), now_ms = now / UINT64_C(1000000);
    uint64_t player = 0;
    SwmpGearSet gear = {0};
    bool changed = lobby_generation != roster.lobby_generation;
    if (changed) {
        local_motion_at = 0;
        local_shield_at = 0;
        local_shield_break = false;
        local_jumping = false;
        local_armor_at = 0;
        memset(&local_armor, 0, sizeof local_armor);
    }
    SwmpPlayerSnapshot local = roster.lobby_generation ? sample_player(cpu, now, &player, &gear) : (SwmpPlayerSnapshot){0};
    uint64_t context = local.area_id ? spawn_context() : 0;
    if (!context) {
        local = (SwmpPlayerSnapshot){0};
        player = 0;
        local_shield_at = 0;
        local_shield_break = false;
        local_jumping = false;
    }
    bool world_changed = local_context != context || local_actor != player || local_area != local.area_id;
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
        const SwmpPosition *position = &roster.positions[peer];
        Pose pose = snapshot_pose(&position->snapshot);
        uint64_t generation = roster.remotes.generation[peer];
        bool has_pose = generation && position->snapshot.area_id == local.area_id &&
            swmp_position_fresh(position, now_ms) && valid_position(pose.position);
        float distance_sq = has_pose ? distance_squared(&local, &pose) : INFINITY;
        float radius = slot->generation == generation ? retain_radius : spawn_radius;
        bool desired = context && distance_sq <= radius * radius;
        if (slot->generation && (changed || world_changed || slot->generation != generation || !desired)) {
            const char *reason = changed || slot->generation != generation ? "roster changed" :
                !context ? "local world unavailable" : world_changed ? "local world changed" :
                !has_pose ? "remote position unavailable" : "out of range";
            retire(cpu, peer, now, reason);
        } else if (slot->generation) {
            queue_playback(slot, position, now_ms);
            SwmpPlayerSnapshot sample = sample_playback(slot, now_ms, &pose);
            poll_actor(cpu, peer, now, &pose, &sample);
            update_gear(cpu, peer, now, context, &sample, &roster.equipment[peer], pose.action);
        } else if (desired && now >= slot->deadline) {
            uint64_t addr = storage + peer * SLOT_SIZE;
            bool proxy = fs_multiplayer_proxy_ready();
            if (!proxy && local.area_id != GAME_PLACE_MAIN_FIELD_AREA) continue;
            if (!proxy) pose.yaw = pose.roll = 0;
            if (request_actor(cpu, addr, context, &pose, proxy ? SWMP_PROXY_ACTOR : placeholder_name, 0)) {
                *slot = (Placeholder){.generation = generation, .deadline = now + request_ns, .proxy = proxy,
                    .applied = pose, .animation = SWMP_PROXY_ACTION_COUNT};
                queue_playback(slot, position, now_ms);
            } else {
                retire(cpu, peer, now, "creation rejected");
            }
        }
    }
#else
    (void)cpu;
#endif
}
