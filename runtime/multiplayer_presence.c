#include "cpu.h"

#ifdef SW_MULTIPLAYER
#include "control_cells.h"
#include "game_sites.h"
#include "hle.h"
#include "multiplayer.h"

#include <math.h>

// Per slot: native creation request, retained actor index, temporary ordinary actor link.
enum { INDEX_OFFSET = 0x10, LINK_OFFSET = 0x20, SLOT_SIZE = 0x40 };
static const uint64_t retry_ns = UINT64_C(1000000000);
static const uint64_t request_ns = UINT64_C(10000000000);
static const char placeholder_name[] = "Npc_TripMaster_00";

typedef struct {
    uint64_t generation, deadline;
    bool retained, visible;
} Placeholder;

// Owned by the guest menu update thread. The request address must stay fixed while the game owns it.
static Placeholder slots[SWMP_MAX_PEERS];
static uint64_t storage, lobby_generation;

RT_ON_RESET(multiplayer_presence_reset) {
    storage = lobby_generation = 0;
    memset(slots, 0, sizeof slots);
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

static void retire(Cpu *cpu, uint8_t peer, uint64_t now) {
    Placeholder *slot = &slots[peer];
    uint64_t addr = storage + peer * SLOT_SIZE;
    if (slot->retained) {
        uint64_t actor = resolve(cpu, addr);
        if (actor) call(cpu, g_site.mp_presence_delete, actor, 2, 0);
        call(cpu, g_site.actor_link_release, addr + LINK_OFFSET, 0, 0);
        call(cpu, g_site.actor_index_release, addr + INDEX_OFFSET, 0, 0);
    }
    call(cpu, g_site.mp_presence_release, addr, 0, 0);
    if (slot->visible) LOG("[multiplayer] placeholder removed for peer %u\n", peer);
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

static bool test_area_loaded(void) {
    // A small test area outside the Shrine of Resurrection, around the existing Plateau test warp.
    double p[3];
    return control_cell_get(&control_cell_player_pos, p) &&
        fabs(p[0] + 1041.6) < 10 && fabs(p[1] - 250.2) < 6 && fabs(p[2] - 1812.1) < 10;
}

static bool request_actor(Cpu *source, uint64_t addr, uint64_t context, unsigned position) {
    // Spread the seven test placements across two rows in front of the default Plateau camera.
    float forward = position < 4 ? 5.0f : 9.0f;
    float right = (float)(position % 4) * 2.3f - 2.3f;
    float matrix[12] = {
        1, 0, 0, -1041.6f + 0.65f * forward + 0.76f * right,
        0, 1, 0, position < 4 ? 251.6f : 253.3f,
        0, 0, 1, 1812.1f - 0.76f * forward + 0.65f * right,
    };
    Cpu cpu = *source;
    cpu.sp -= 0x80;
    uint64_t name = cpu.sp + 0x10, transform = cpu.sp + 0x30;
    memcpy((void *)(uintptr_t)name, placeholder_name, sizeof placeholder_name);
    memcpy((void *)(uintptr_t)transform, matrix, sizeof matrix);
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

static void poll_actor(Cpu *cpu, uint8_t peer, uint64_t now) {
    Placeholder *slot = &slots[peer];
    uint64_t addr = storage + peer * SLOT_SIZE;
    if (slot->retained) {
        uint64_t actor = resolve(cpu, addr);
        bool active = actor && LD8(actor + 0x64) == 1 && !(LD32(actor + 0x78) & 1);
        call(cpu, g_site.actor_link_release, addr + LINK_OFFSET, 0, 0);
        if (active && !slot->visible) {
            slot->visible = true;
            LOG("[multiplayer] placeholder ready for peer %u\n", peer);
        } else if (!active && (slot->visible || now >= slot->deadline)) {
            retire(cpu, peer, now);
        }
    } else if (call(cpu, g_site.mp_presence_ready, addr, 0, 0) & 1) {
        // Taking the completed actor clears its creation suspension. Keep an indexed reference for deletion.
        uint64_t actor = call(cpu, g_site.mp_presence_take, addr, 0, 0);
        if (guest_range(actor, 0x7c) && call(cpu, g_site.actor_index_assign, addr + INDEX_OFFSET, actor, 1)) {
            slot->retained = true;
        } else {
            if (guest_range(actor, 0x7c)) call(cpu, g_site.mp_presence_delete, actor, 2, 0);
            retire(cpu, peer, now);
        }
    } else if (now >= slot->deadline || (call(cpu, g_site.mp_presence_failed, addr, 0, 0) & 1)) {
        retire(cpu, peer, now);
    }
}

#endif

void rt_mp_presence_tick(Cpu *cpu) {
#ifdef SW_MULTIPLAYER
    SwmpM1Roster roster = swmp_m1_roster();
    if (!roster.lobby_generation && !storage) return;
    if (!g_site.mp_presence_create || !g_site.actor_index_assign ||
        (cpu->sp & 15) || !guest_range(cpu->sp - 0x80, 0xe0)) return;

    uint64_t context = roster.lobby_generation && test_area_loaded() ? spawn_context() : 0;
    if (!storage) {
        bool any = false;
        for (uint8_t peer = 0; peer < SWMP_MAX_PEERS; peer++) any |= roster.remotes.generation[peer] != 0;
        if (!any || !context) return;
        storage = (uint64_t)(uintptr_t)hle_alloc(SWMP_MAX_PEERS * SLOT_SIZE);
        if (!storage) return;
        memset((void *)(uintptr_t)storage, 0, SWMP_MAX_PEERS * SLOT_SIZE);
        for (uint8_t peer = 0; peer < SWMP_MAX_PEERS; peer++)
            ST32(storage + peer * SLOT_SIZE + INDEX_OFFSET + 8, UINT32_MAX);
    }

    uint64_t now = plat_now_ns();
    bool changed = lobby_generation != roster.lobby_generation;
    lobby_generation = roster.lobby_generation;
    for (uint8_t peer = 0; peer < SWMP_MAX_PEERS; peer++) {
        Placeholder *slot = &slots[peer];
        uint64_t desired = context ? roster.remotes.generation[peer] : 0;
        if (slot->generation && (changed || slot->generation != desired)) {
            retire(cpu, peer, now);
        } else if (slot->generation) {
            poll_actor(cpu, peer, now);
        } else if (desired && now >= slot->deadline) {
            uint64_t addr = storage + peer * SLOT_SIZE;
            if (request_actor(cpu, addr, context, peer - (peer > roster.local_peer))) {
                *slot = (Placeholder){.generation = desired, .deadline = now + request_ns};
            } else {
                retire(cpu, peer, now);
            }
        }
    }
#else
    (void)cpu;
#endif
}
