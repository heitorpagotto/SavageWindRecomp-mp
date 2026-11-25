#include "multiplayer_assets.h"

#ifdef SW_MULTIPLAYER
#include "services/archive.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

static const char template_actor[] = "Obj_RockGray_A_M_01";
static const char model_archive[] = "Obj_RockGray_A";
static const char link_model[] = "Link";

#define VISUAL(slot, source, model, actor, label, bundle) {slot, model, actor, label, \
    "Actor/Pack/" source ".sbactorpack", bundle, "Actor/Pack/" actor ".sbactorpack", \
    "Actor/ActorLink/" actor ".bxml", source, "Actor/ModelList/" source ".bmodellist"}
#define GEAR(slot, model, actor, label) VISUAL(slot, model, model, actor, label, NULL)
#define ARROW(type, source, model, actor, label) \
    [SWMP_ARROW_ASSET_BASE + SWMP_ARROW_##type] = VISUAL(SWMP_NOCKED_ARROW_SLOT, source, model, actor, label, "Pack/TitleBG.pack")
const SwmpGearAsset swmp_gear_assets[SWMP_VISUAL_ASSET_COUNT] = {
    [SWMP_GEAR_WOODCUTTERS_AXE] = GEAR(SWMP_GEAR_WEAPON, "Weapon_Lsword_032", "SwmpRemoteAxe", "axe"),
    [SWMP_GEAR_WOODEN_SHIELD] = GEAR(SWMP_GEAR_SHIELD, "Weapon_Shield_001", "SwmpRemoteShield", "shield"),
    [SWMP_GEAR_TRAVELERS_BOW] = GEAR(SWMP_GEAR_BOW, "Weapon_Bow_001", "SwmpRemoteBow", "bow"),
    [SWMP_GEAR_TRAVELERS_SWORD] = GEAR(SWMP_GEAR_WEAPON, "Weapon_Sword_001", "SwmpRemoteSword", "sword"),
    [SWMP_GEAR_TRAVELERS_SPEAR] = GEAR(SWMP_GEAR_WEAPON, "Weapon_Spear_001", "SwmpRemoteSpear", "spear"),
    [SWMP_GEAR_TRAVELERS_CLAYMORE] = GEAR(SWMP_GEAR_WEAPON, "Weapon_Lsword_001", "SwmpRemoteClaymore", "claymore"),
    [SWMP_GEAR_SOLDIERS_SHIELD] = GEAR(SWMP_GEAR_SHIELD, "Weapon_Shield_002", "SwmpRemoteSoldierShield", "soldier shield"),
    [SWMP_GEAR_SOLDIERS_BOW] = GEAR(SWMP_GEAR_BOW, "Weapon_Bow_002", "SwmpRemoteSoldierBow", "soldier bow"),
    [SWMP_PARAGLIDER_ASSET] = VISUAL(SWMP_PARAGLIDER_SLOT, "PlayerStole2", "Item_Parastole2",
                                    "SwmpRemoteGlider", "glider", "Pack/TitleBG.pack"),
    ARROW(NORMAL, "NormalArrow", "Weapon_Arrow_001", "SwmpRemoteArrow", "nocked arrow"),
    ARROW(FIRE, "FireArrow", "Weapon_Arrow_005", "SwmpRemoteFireArrow", "nocked fire arrow"),
    ARROW(ICE, "IceArrow", "Weapon_Arrow_006", "SwmpRemoteIceArrow", "nocked ice arrow"),
    ARROW(SHOCK, "ElectricArrow", "Weapon_Arrow_004", "SwmpRemoteShockArrow", "nocked shock arrow"),
    ARROW(BOMB, "BombArrow_A", "Weapon_Arrow_002", "SwmpRemoteBombArrow", "nocked bomb arrow"),
    ARROW(ANCIENT, "AncientArrow", "Weapon_Arrow_007", "SwmpRemoteAncientArrow", "nocked ancient arrow"),
};
#undef ARROW
#undef GEAR
#undef VISUAL

static uint32_t read32(const uint8_t *at) {
    uint32_t value;
    memcpy(&value, at, 4);
    return value;
}

static void write32(uint8_t *at, uint32_t value) {
    memcpy(at, &value, 4);
}

static uint32_t crc(const char *name) {
    return (uint32_t)crc32(0, (const Bytef *)name, (uInt)strlen(name));
}

static size_t align4(size_t n) { return (n + 3) & ~(size_t)3; }

typedef struct {
    uint8_t *data;
    size_t original_size, length, capacity;
} Byml;

typedef struct {
    uint8_t *head, *values;
    uint32_t count;
} BymlNode;

enum { STRING = 0xa0, ARRAY = 0xc0, DICT = 0xc1, STRINGS = 0xc2, INT = 0xd1, UINT = 0xd3 };

static BymlNode node(const Byml *b, uint32_t offset, unsigned type) {
    if ((offset & 3) || offset > b->original_size - 4 || b->data[offset] != type)
        return (BymlNode){0};
    uint8_t *head = b->data + offset;
    uint32_t count = read32(head) >> 8;
    size_t prefix = 4 + (type == ARRAY ? align4(count) : 0);
    size_t stride = type == DICT ? 8 : 4;
    if (prefix + stride * count + (type == STRINGS ? 4 : 0) > b->original_size - offset)
        return (BymlNode){0};
    return (BymlNode){head, head + prefix, count};
}

static const char *string_at(const Byml *b, BymlNode table, uint32_t index) {
    if (index >= table.count) return NULL;
    uint32_t start = read32(table.values + 4 * index), end = read32(table.values + 4 * (index + 1));
    if (start < 8 + table.count * 4 || start >= end ||
        end > b->original_size - (size_t)(table.head - b->data) || table.head[end - 1])
        return NULL;
    return (const char *)table.head + start;
}

static uint8_t *field(const Byml *b, BymlNode keys, BymlNode dict, const char *name, unsigned type) {
    for (uint32_t i = 0; i < dict.count; i++) {
        uint8_t *entry = dict.values + 8 * i;
        const char *key = string_at(b, keys, read32(entry) & 0xffffff);
        if (entry[3] == type && key && !strcmp(key, name)) return entry + 4;
    }
    return NULL;
}

static bool shift_string_refs(const Byml *b, uint32_t offset, unsigned type, uint32_t insert,
                              uint8_t *visited, unsigned depth) {
    BymlNode n = node(b, offset, type);
    if (!n.head || depth == 16) return false;
    if (visited[offset / 4]) return true;
    visited[offset / 4] = 1;
    for (uint32_t i = 0; i < n.count; i++) {
        uint8_t *value = n.values + i * (type == ARRAY ? 4 : 8) + (type == DICT ? 4 : 0);
        unsigned tag = type == ARRAY ? n.head[4 + i] : value[-1];
        uint32_t ref = read32(value);
        if (tag == STRING && ref >= insert) write32(value, ref + 1);
        if ((tag == ARRAY || tag == DICT) && !shift_string_refs(b, ref, tag, insert, visited, depth + 1))
            return false;
    }
    return true;
}

static uint8_t *append(Byml *b, size_t size) {
    size_t offset = align4(b->length);
    if (offset + size > b->capacity) return NULL;
    b->length = offset + size;
    return b->data + offset;
}

static uint8_t *insert_string(Byml *b, BymlNode strings, uint32_t index, const char *alias) {
    uint32_t start = read32(strings.values + 4 * index);
    uint32_t end = read32(strings.values + 4 * strings.count);
    size_t extra = strlen(alias) + 1;
    uint8_t *out = append(b, end + 4 + extra);
    if (!out) return NULL;
    write32(out, ((strings.count + 1) << 8) | STRINGS);
    for (uint32_t i = 0; i <= strings.count; i++) {
        uint32_t old = read32(strings.values + 4 * i);
        write32(out + 4 + 4 * (i + (i >= index)), old + 4 + (i >= index ? (uint32_t)extra : 0));
    }
    uint32_t first = read32(strings.values);
    write32(out + 4 + 4 * index, start + 4);
    memcpy(out + first + 4, strings.head + first, start - first);
    memcpy(out + start + 4, alias, extra);
    memcpy(out + start + 4 + extra, strings.head + start, end - start);
    return out;
}

static uint8_t *insert_array(Byml *b, BymlNode original, uint32_t index, uint8_t type, uint32_t value) {
    uint32_t count = original.count + 1;
    uint8_t *out = append(b, 4 + align4(count) + 4 * count);
    if (!out) return NULL;
    write32(out, (count << 8) | ARRAY);
    uint8_t *types = out + 4, *values = types + align4(count);
    memcpy(types, original.head + 4, index);
    types[index] = type;
    memcpy(types + index + 1, original.head + 4 + index, original.count - index);
    memcpy(values, original.values, index * 4);
    write32(values + index * 4, value);
    memcpy(values + (index + 1) * 4, original.values + index * 4, (original.count - index) * 4);
    return out;
}

static bool add_actor(Byml *b, const char *alias, const char *model_name, uint32_t heap_size) {
    if (memcmp(b->data, "YB\x02\0", 4)) return false;
    BymlNode keys = node(b, read32(b->data + 4), STRINGS);
    BymlNode strings = node(b, read32(b->data + 8), STRINGS);
    uint32_t root_offset = read32(b->data + 12);
    BymlNode root = node(b, root_offset, DICT);
    uint8_t *actors_ref = field(b, keys, root, "Actors", ARRAY);
    uint8_t *hashes_ref = field(b, keys, root, "Hashes", ARRAY);
    if (!actors_ref || !hashes_ref || !strings.count) return false;
    BymlNode actors = node(b, read32(actors_ref), ARRAY), hashes = node(b, read32(hashes_ref), ARRAY);
    if (!actors.count || actors.count != hashes.count) return false;

    uint32_t name_index = 0, model_index = strings.count;
    for (uint32_t i = 0; i < strings.count; i++) {
        const char *s = string_at(b, strings, i);
        if (!s) return false;
        int order = strcmp(s, alias);
        if (!order) return false;
        if (order < 0) name_index++;
        if (!strcmp(s, model_name)) model_index = i;
    }
    if (model_index == strings.count) return false;
    model_index += model_index >= name_index;

    uint32_t insert = 0, alias_crc = crc(alias), stock_crc = crc(template_actor);
    BymlNode stock = {0};
    for (uint32_t i = 0; i < hashes.count; i++) {
        uint32_t hash = read32(hashes.values + 4 * i);
        if (hash == alias_crc) return false;
        if (hash < alias_crc) insert++;
        if (hash == stock_crc && actors.head[4 + i] == DICT)
            stock = node(b, read32(actors.values + 4 * i), DICT);
    }
    uint8_t *name = field(b, keys, stock, "name", STRING);
    uint8_t *bfres = field(b, keys, stock, "bfres", STRING);
    uint8_t *model = field(b, keys, stock, "mainModel", STRING);
    uint8_t *heap = field(b, keys, stock, "instSize", INT);
    const char *stock_name = name ? string_at(b, strings, read32(name)) : NULL;
    if (!stock_name || strcmp(stock_name, template_actor) || !bfres || !model || !heap) return false;

    // Actor entries can share nodes and refer to strings by index. Visit each node once.
    uint8_t *visited = calloc(b->original_size / 4 + 1, 1);
    if (!visited) return false;
    bool ok = shift_string_refs(b, root_offset, DICT, name_index, visited, 0);
    free(visited);
    if (!ok) return false;

    uint8_t *new_strings = insert_string(b, strings, name_index, alias);
    size_t actor_size = 4 + 8 * stock.count, root_size = 4 + 8 * root.count;
    uint8_t *actor = append(b, actor_size);
    if (!actor || !new_strings) return false;
    memcpy(actor, stock.head, actor_size);
    write32(actor + (name - stock.head), name_index);
    write32(actor + (bfres - stock.head), model_index);
    write32(actor + (model - stock.head), model_index);
    write32(actor + (heap - stock.head), heap_size);

    uint8_t *new_actors = insert_array(b, actors, insert, DICT, (uint32_t)(actor - b->data));
    uint8_t *new_hashes = insert_array(b, hashes, insert, alias_crc > INT32_MAX ? UINT : INT, alias_crc);
    uint8_t *new_root = append(b, root_size);
    if (!new_actors || !new_hashes || !new_root) return false;
    memcpy(new_root, root.head, root_size);
    write32(new_root + (actors_ref - root.head), (uint32_t)(new_actors - b->data));
    write32(new_root + (hashes_ref - root.head), (uint32_t)(new_hashes - b->data));
    write32(b->data + 8, (uint32_t)(new_strings - b->data));
    write32(b->data + 12, (uint32_t)(new_root - b->data));
    return true;
}

uint8_t *swmp_proxy_actor_info(const uint8_t *src, size_t size, unsigned gear_mask, size_t *out_size) {
    size_t n;
    uint8_t *raw = src ? yaz0_unpack(src, size, &n) : NULL;
    if (!raw || n < 16 || n > 16 * 1024 * 1024) {
        free(raw);
        return NULL;
    }
    // Each alias appends a copy of the string, actor and hash tables.
    Byml b = {.original_size = n, .length = n, .capacity = 5 * n + 512};
    b.data = calloc(1, b.capacity);
    if (b.data) memcpy(b.data, raw, n);
    free(raw);
    if (!b.data || !add_actor(&b, SWMP_PROXY_ACTOR, link_model, 2 * 1024 * 1024)) {
        free(b.data);
        return NULL;
    }
    for (unsigned i = 1; i < SWMP_VISUAL_ASSET_COUNT; i++) {
        if (!(gear_mask & (1u << i))) continue;
        b.original_size = b.length;
        const SwmpGearAsset *gear = &swmp_gear_assets[i];
        if (!add_actor(&b, gear->actor, gear->model, 256 * 1024)) {
            free(b.data);
            return NULL;
        }
    }
    *out_size = b.length;
    return b.data;
}

typedef struct {
    const char *name;
    const uint8_t *data;
    size_t size;
} PackFile;

static uint32_t sarc_hash(const char *name) {
    uint32_t hash = 0;
    for (; *name; name++) hash = hash * 0x65 + (unsigned char)*name;
    return hash;
}

static int compare_pack_files(const void *a, const void *b) {
    uint32_t first = sarc_hash(((const PackFile *)a)->name);
    uint32_t second = sarc_hash(((const PackFile *)b)->name);
    return (first > second) - (first < second);
}

static uint8_t *pack_files(PackFile *files, unsigned count, size_t *size) {
    qsort(files, count, sizeof *files, compare_pack_files);
    size_t names = 0x28 + 16 * count, names_size = 0, data_size = 0;
    for (unsigned i = 0; i < count; i++) {
        names_size += align4(strlen(files[i].name) + 1);
        data_size += align4(files[i].size);
    }
    size_t data = (names + names_size + 63) & ~(size_t)63;
    *size = data + data_size;
    uint8_t *out = calloc(1, *size);
    if (!out) return NULL;

    memcpy(out, "SARC\x14\0\xff\xfe", 8);
    write32(out + 8, (uint32_t)*size);
    write32(out + 12, (uint32_t)data);
    write32(out + 16, 0x100);
    memcpy(out + 20, "SFAT\x0c\0", 6);
    out[26] = (uint8_t)count;
    write32(out + 28, 0x65);
    memcpy(out + names - 8, "SFNT\x08\0\0\0", 8);
    for (unsigned i = 0, name_offset = 0, data_offset = 0; i < count; i++) {
        PackFile f = files[i];
        uint8_t *entry = out + 0x20 + 16 * i;
        write32(entry, sarc_hash(f.name));
        write32(entry + 4, 0x01000000 | name_offset / 4);
        write32(entry + 8, data_offset);
        write32(entry + 12, data_offset + (uint32_t)f.size);
        memcpy(out + names + name_offset, f.name, strlen(f.name) + 1);
        memcpy(out + data + data_offset, f.data, f.size);
        name_offset += (unsigned)align4(strlen(f.name) + 1);
        data_offset += (unsigned)align4(f.size);
    }
    return out;
}

static uint8_t *aamp_param(uint8_t *data, size_t size, const char *key) {
    uint32_t hash = crc(key);
    if (size < 8 || memcmp(data, "AAMP", 4)) return NULL;
    for (size_t i = 0; i <= size - 8; i += 4)
        if (read32(data + i) == hash && data[i + 7] == 20) return data + i;
    return NULL;
}

static bool set_dummy_physics(uint8_t *data, size_t size) {
    uint8_t *ai = aamp_param(data, size, "AIProgramUser");
    uint8_t *physics = aamp_param(data, size, "PhysicsUser");
    if (!ai || !physics) return false;
    size_t dummy = (size_t)(ai - data) + (read32(ai + 4) & 0xffffff) * 4;
    if (dummy + sizeof "Dummy" > size || memcmp(data + dummy, "Dummy", sizeof "Dummy") ||
        data + dummy <= physics) return false;
    // AAMP string parameters encode the type and a word offset from the parameter entry.
    write32(physics + 4, (20u << 24) | (uint32_t)((data + dummy - physics) / 4));
    return true;
}

static bool replace_model_name(uint8_t *data, size_t size, const char *old) {
    size_t len = strlen(old) + 1;
    bool found = false;
    for (size_t i = 0; i + len <= size; i += 4) {
        if (memcmp(data + i, old, len)) continue;
        memset(data + i, 0, len);
        memcpy(data + i, link_model, sizeof link_model);
        found = true;
    }
    return found;
}

typedef enum {
    PROXY_GEAR_NONE,
    PROXY_GEAR_LSWORD,
    PROXY_GEAR_SWORD,
    PROXY_GEAR_SPEAR,
    PROXY_GEAR_BOW,
    PROXY_GEAR_SHIELD,
} ProxyGear;

typedef struct {
    const char *state, *file, *path, *clip, *native;
    ProxyGear gear;
} ProxyAction;

// Only references to installed clips go into the alias.
#define ACTION(state, clip, native, gear) {#state, "SwmpProxy" #state, "Actor/AS/SwmpProxy" #state ".bas", clip, native, gear}
static const ProxyAction proxy_actions[SWMP_PROXY_ACTION_COUNT] = {
    [SWMP_PROXY_IDLE] = ACTION(Idle, "Nml_Wait", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_JUMP] = ACTION(Jump, "Nml_Wait_Jump", "Jump", PROXY_GEAR_NONE),
    [SWMP_PROXY_WALK] = ACTION(Walk, "Nml_Move_Walk", "Move", PROXY_GEAR_NONE),
    [SWMP_PROXY_DASH] = ACTION(Dash, "Nml_Move_Dash", "Dash", PROXY_GEAR_NONE),
    [SWMP_PROXY_RUN] = ACTION(Run, "Nml_Move_Run", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_LAND] = ACTION(Land, "Nml_Wait_Land", "Land", PROXY_GEAR_NONE),
    [SWMP_PROXY_AXE_DRAW] = ACTION(AxeDraw, "Equip_Lsword_On_Natural", "WeaponEquipOn", PROXY_GEAR_LSWORD),
    [SWMP_PROXY_AXE_SWING] = ACTION(AxeSwing, "Lsword_Attack_S1", "CutS1", PROXY_GEAR_LSWORD),
    [SWMP_PROXY_AXE_FINISH] = ACTION(AxeFinish, "Lsword_Attack_SF", "CutSF", PROXY_GEAR_LSWORD),
    [SWMP_PROXY_AXE_SPIN] = ACTION(AxeSpin, "Lsword_Attack_Charge", "CutCharge", PROXY_GEAR_LSWORD),
    [SWMP_PROXY_AXE_READY] = ACTION(AxeReady, "A_Pose_Lsword_Wait", NULL, PROXY_GEAR_LSWORD),
    [SWMP_PROXY_AXE_STOW] = ACTION(AxeStow, "Equip_Lsword_Off_Natural", "WeaponEquipOff", PROXY_GEAR_LSWORD),
    [SWMP_PROXY_CROUCH_IDLE] = ACTION(CrouchIdle, "Squat_Wait_FootL", "SquatWait", PROXY_GEAR_NONE),
    [SWMP_PROXY_CROUCH_MOVE] = ACTION(CrouchMove, "Squat_Move_Walk", "SquatMove", PROXY_GEAR_NONE),
    [SWMP_PROXY_SHIELD_GUARD] = ACTION(ShieldGuard, "Shield_Guard_Wait", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_BOW_RELOAD] = ACTION(BowReload, "Bow_Attack_Load", "BowReload", PROXY_GEAR_BOW),
    [SWMP_PROXY_BOW_AIM] = ACTION(BowAim, "Bow_Attack_Charge", "BowCharge", PROXY_GEAR_BOW),
    [SWMP_PROXY_BOW_SHOOT] = ACTION(BowShoot, "Bow_Attack_Shoot", "BowShoot", PROXY_GEAR_BOW),
    [SWMP_PROXY_SWIM_WAIT] = ACTION(SwimWait, "Swim_Wait", "SwimWait", PROXY_GEAR_NONE),
    [SWMP_PROXY_SWIM_MOVE] = ACTION(SwimMove, "Swim_Move_Slow_F", "SwimMove", PROXY_GEAR_NONE),
    [SWMP_PROXY_SWIM_DASH] = ACTION(SwimDash, "Swim_Move_Dash", "SwimDash", PROXY_GEAR_NONE),
    [SWMP_PROXY_CLIMB_WAIT] = ACTION(ClimbWait, "Climb_Wall_Wait_FootL", "ClimbWait", PROXY_GEAR_NONE),
    [SWMP_PROXY_CLIMB_MOVE] = ACTION(ClimbMove, "Climb_Wall_Move_U", "ClimbMove", PROXY_GEAR_NONE),
    [SWMP_PROXY_CLIMB_JUMP] = ACTION(ClimbJump, "Climb_Wall_Jump_U_FootL", "ClimbJump", PROXY_GEAR_NONE),
    [SWMP_PROXY_CLIMB_RIGHT] = ACTION(ClimbRight, "Climb_Wall_Move_R", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_CLIMB_UP_RIGHT] = ACTION(ClimbUpRight, "Climb_Wall_Move_UR", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_CLIMB_UP_LEFT] = ACTION(ClimbUpLeft, "Climb_Wall_Move_UL", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_CLIMB_LEFT] = ACTION(ClimbLeft, "Climb_Wall_Move_L", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_CLIMB_DOWN_LEFT] = ACTION(ClimbDownLeft, "Climb_Wall_Move_DL", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_CLIMB_DOWN] = ACTION(ClimbDown, "Climb_Wall_Move_D", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_CLIMB_DOWN_RIGHT] = ACTION(ClimbDownRight, "Climb_Wall_Move_DR", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_CLIMB_ENTER] = ACTION(ClimbEnter, "Wall_Dash_FootL", "WallDash", PROXY_GEAR_NONE),
    [SWMP_PROXY_CLIMB_MANTLE] = ACTION(ClimbMantle, "Climb_Wall_Ed_Edge_L", "ClimbEd", PROXY_GEAR_NONE),
    [SWMP_PROXY_GLIDE_DEPLOY] = ACTION(GlideDeploy, "Equip_Float_On_Fall", "ParaEquipOn", PROXY_GEAR_NONE),
    [SWMP_PROXY_GLIDE] = ACTION(Glide, "Float_Float", "ParashawlGlide", PROXY_GEAR_NONE),
    [SWMP_PROXY_FALL] = ACTION(Fall, "Fall", "Fall", PROXY_GEAR_NONE),
    [SWMP_PROXY_CLIMB_JUMP_RIGHT] = ACTION(ClimbJumpRight, "Climb_Wall_Jump_R_FootR", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_CLIMB_JUMP_UP_RIGHT] = ACTION(ClimbJumpUpRight, "Climb_Wall_Jump_UR_FootL", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_CLIMB_JUMP_UP_LEFT] = ACTION(ClimbJumpUpLeft, "Climb_Wall_Jump_UL_FootL", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_CLIMB_JUMP_LEFT] = ACTION(ClimbJumpLeft, "Climb_Wall_Jump_L_FootL", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_SWIM_FAST] = ACTION(SwimFast, "Swim_Move_Fast_F", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_SWIM_SLOW_LEFT] = ACTION(SwimSlowLeft, "Swim_Move_Slow_F_Curve_L", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_SWIM_SLOW_RIGHT] = ACTION(SwimSlowRight, "Swim_Move_Slow_F_Curve_R", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_SWIM_FAST_LEFT] = ACTION(SwimFastLeft, "Swim_Move_Fast_F_Curve_L", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_SWIM_FAST_RIGHT] = ACTION(SwimFastRight, "Swim_Move_Fast_F_Curve_R", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_SWIM_LOCK_FORWARD] = ACTION(SwimLockForward, "Swim_Lockon_Move_F", "SwimAtnMove", PROXY_GEAR_NONE),
    [SWMP_PROXY_SWIM_LOCK_BACK] = ACTION(SwimLockBack, "Swim_Lockon_Move_B", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_SWIM_LOCK_LEFT] = ACTION(SwimLockLeft, "Swim_Lockon_Move_L", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_SWIM_LOCK_RIGHT] = ACTION(SwimLockRight, "Swim_Lockon_Move_R", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_SWORD_DRAW] = ACTION(SwordDraw, "Equip_Sword_On_Natural", "WeaponEquipOn", PROXY_GEAR_SWORD),
    [SWMP_PROXY_SWORD_READY] = ACTION(SwordReady, "A_Pose_Sword_Wait", NULL, PROXY_GEAR_SWORD),
    [SWMP_PROXY_SWORD_STOW] = ACTION(SwordStow, "Equip_Sword_Off_Natural", "WeaponEquipOff", PROXY_GEAR_SWORD),
    [SWMP_PROXY_SWORD_SWING] = ACTION(SwordSwing, "Sword_Attack_S1", "CutS1", PROXY_GEAR_SWORD),
    [SWMP_PROXY_SWORD_CHAIN_2] = ACTION(SwordChain2, "Sword_Attack_S2", "CutS2", PROXY_GEAR_SWORD),
    [SWMP_PROXY_SWORD_CHAIN_3] = ACTION(SwordChain3, "Sword_Attack_S3", "CutS3", PROXY_GEAR_SWORD),
    [SWMP_PROXY_SWORD_FINISH] = ACTION(SwordFinish, "Sword_Attack_SF", "CutSF", PROXY_GEAR_SWORD),
    [SWMP_PROXY_SPEAR_DRAW] = ACTION(SpearDraw, "Equip_Spear_On_Natural", "WeaponEquipOn", PROXY_GEAR_SPEAR),
    [SWMP_PROXY_SPEAR_READY] = ACTION(SpearReady, "A_Pose_Spear_Wait", NULL, PROXY_GEAR_SPEAR),
    [SWMP_PROXY_SPEAR_STOW] = ACTION(SpearStow, "Equip_Spear_Off_Natural", "WeaponEquipOff", PROXY_GEAR_SPEAR),
    [SWMP_PROXY_SPEAR_THRUST] = ACTION(SpearThrust, "Spear_Attack_S1", "CutS1", PROXY_GEAR_SPEAR),
    [SWMP_PROXY_SPEAR_CHAIN_2] = ACTION(SpearChain2, "Spear_Attack_S2", "CutS2", PROXY_GEAR_SPEAR),
    [SWMP_PROXY_SPEAR_FINISH] = ACTION(SpearFinish, "Spear_Attack_SF", "CutSF", PROXY_GEAR_SPEAR),
    // The native spear combo reuses the same clip for three separately activated thrusts.
    [SWMP_PROXY_SPEAR_CHAIN_3] = ACTION(SpearChain3, "Spear_Attack_S2", "CutS3", PROXY_GEAR_SPEAR),
    [SWMP_PROXY_SPEAR_CHAIN_4] = ACTION(SpearChain4, "Spear_Attack_S2", "CutS4", PROXY_GEAR_SPEAR),
    [SWMP_PROXY_AXE_CHARGE_READY] = ACTION(AxeChargeReady, "Lsword_Charge_Wait", "CutChargeLswordSpin", PROXY_GEAR_LSWORD),
    [SWMP_PROXY_SWORD_CHARGE_READY] = ACTION(SwordChargeReady, "Sword_Charge_Wait", "CutChargeWait", PROXY_GEAR_SWORD),
    [SWMP_PROXY_SWORD_CHARGE] = ACTION(SwordCharge, "Sword_Attack_Charge", "CutCharge", PROXY_GEAR_SWORD),
    [SWMP_PROXY_SPEAR_CHARGE_READY] = ACTION(SpearChargeReady, "Spear_Charge_Wait", "CutChargeWait", PROXY_GEAR_SPEAR),
    [SWMP_PROXY_SPEAR_CHARGE] = ACTION(SpearCharge, "Spear_Attack_Charge_Rush", "CutCharge", PROXY_GEAR_SPEAR),
    [SWMP_PROXY_SPEAR_CHARGE_FINISH] = ACTION(SpearChargeFinish, "Spear_Attack_SF", "CutChargeF", PROXY_GEAR_SPEAR),
    [SWMP_PROXY_SHIELD_PARRY] = ACTION(ShieldParry, "Shield_Guard_Just", "GuardJust", PROXY_GEAR_SHIELD),
    [SWMP_PROXY_SHIELD_HIT] = ACTION(ShieldHit, "Shield_Guard_Hit", "GuardFailHit", PROXY_GEAR_SHIELD),
    [SWMP_PROXY_SHIELD_BREAK] = ACTION(ShieldBreak, "Shield_Guard_Break", "GuardBreak", PROXY_GEAR_SHIELD),
    [SWMP_PROXY_BOW_DRAW] = ACTION(BowDraw, "Equip_Bow_On_Natural", NULL, PROXY_GEAR_BOW),
    [SWMP_PROXY_BOW_STOW] = ACTION(BowStow, "Equip_Bow_Off_Natural", NULL, PROXY_GEAR_BOW),
    [SWMP_PROXY_BOW_READY] = ACTION(BowReady, "Bow_Lockon_Wait", NULL, PROXY_GEAR_BOW),
    [SWMP_PROXY_STEP] = ACTION(Step, "Nml_Lockon_Step_N_St", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_STEP_LEFT] = ACTION(StepLeft, "Nml_Lockon_Step_L_St", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_STEP_RIGHT] = ACTION(StepRight, "Nml_Lockon_Step_R_St", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_STEP_FORWARD] = ACTION(StepForward, "Nml_Lockon_Step_F_St", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_STEP_BACK] = ACTION(StepBack, "Nml_Lockon_Step_B_St", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_STEP_LAND] = ACTION(StepLand, "Nml_Lockon_Step_N_Ed", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_STEP_LAND_LEFT] = ACTION(StepLandLeft, "Nml_Lockon_Step_L_Ed", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_STEP_LAND_RIGHT] = ACTION(StepLandRight, "Nml_Lockon_Step_R_Ed", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_STEP_LAND_FORWARD] = ACTION(StepLandForward, "Nml_Lockon_Step_F_Ed", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_STEP_LAND_BACK] = ACTION(StepLandBack, "Nml_Lockon_Step_B_Ed", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_BACK_JUMP] = ACTION(BackJump, "Nml_Lockon_Jump_B_St", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_BACK_JUMP_LAND] = ACTION(BackJumpLand, "Nml_Lockon_Jump_B_Ed", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_DAMAGE_LIGHT] = ACTION(DamageLight, "Nml_Damage_S_F", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_DAMAGE_MEDIUM] = ACTION(DamageMedium, "Nml_Damage_M_F", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_KNOCKDOWN] = ACTION(Knockdown, "Nml_Damage_L_F", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_GET_UP] = ACTION(GetUp, "Nml_Damage_L_F_Standup", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_TUMBLE] = ACTION(Tumble, "Dive_To_Dive", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_TUMBLE_LAND] = ACTION(TumbleLand, "Rolldown_D", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_HARD_LAND] = ACTION(HardLand, "Nml_Damage_Land", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_HARD_LAND_RECOVER] = ACTION(HardLandRecover, "Nml_Damage_Land_Stand", NULL, PROXY_GEAR_NONE),
};
#undef ACTION

const char *swmp_proxy_action_name(SwmpProxyAction action) {
    return proxy_actions[(unsigned)action < SWMP_PROXY_ACTION_COUNT ? action : SWMP_PROXY_IDLE].state;
}

typedef struct {
    ProxyGear family;
    SwmpProxyAction ready;
    int min_hand_y, max_hand_y;
} WeaponAnimation;

static const WeaponAnimation weapon_animations[SWMP_GEAR_COUNT] = {
    [SWMP_GEAR_WOODCUTTERS_AXE] = {PROXY_GEAR_LSWORD, SWMP_PROXY_AXE_READY, 550, 1050},
    [SWMP_GEAR_TRAVELERS_CLAYMORE] = {PROXY_GEAR_LSWORD, SWMP_PROXY_AXE_READY, 550, 1050},
    [SWMP_GEAR_TRAVELERS_SWORD] = {PROXY_GEAR_SWORD, SWMP_PROXY_SWORD_READY, 550, 1050},
    // The spear hangs lower than the other weapons even when drawn.
    [SWMP_GEAR_TRAVELERS_SPEAR] = {PROXY_GEAR_SPEAR, SWMP_PROXY_SPEAR_READY, 525, 950},
};

SwmpProxyAction swmp_proxy_sample_action(const char *state, const char *upper, SwmpGearSet gear,
                                         const SwmpGearPose *weapon_pose, const SwmpGearPose *bow_pose,
                                         bool shield_raised) {
    if (!strcmp(state, "WallDashSt")) return SWMP_PROXY_CLIMB_ENTER;
    if (!strcmp(state, "Step")) return SWMP_PROXY_STEP;
    if (!strcmp(state, "StepLand")) return SWMP_PROXY_STEP_LAND;
    if (!strcmp(state, "BackJump")) return SWMP_PROXY_BACK_JUMP;
    if (!strcmp(state, "BackJumpLand")) return SWMP_PROXY_BACK_JUMP_LAND;
    // Use a frontal clip for each full-body hit reaction. Native hit direction has its own selector.
    if (!strcmp(state, "DamageS")) return SWMP_PROXY_DAMAGE_LIGHT;
    if (!strcmp(state, "DamageM")) return SWMP_PROXY_DAMAGE_MEDIUM;
    if (!strcmp(state, "DamageL")) return SWMP_PROXY_KNOCKDOWN;
    if (!strcmp(state, "DamageLStand")) return SWMP_PROXY_GET_UP;
    if (!strcmp(state, "SlippingDown")) return SWMP_PROXY_TUMBLE;
    if (!strcmp(state, "SlippingDownLand")) return SWMP_PROXY_TUMBLE_LAND;
    if (!strcmp(state, "LandDamage")) return SWMP_PROXY_HARD_LAND;
    if (!strcmp(state, "LandDamageStand")) return SWMP_PROXY_HARD_LAND_RECOVER;
    unsigned id = gear.id[SWMP_GEAR_WEAPON];
    const WeaponAnimation *weapon = &weapon_animations[id < SWMP_GEAR_COUNT ? id : SWMP_GEAR_NONE];
    // The attachment height disambiguates native Wait and filters WeaponEquipOn for a bow draw.
    bool weapon_drawn = weapon_pose && weapon->family &&
        weapon_pose->offset_mm[1] >= weapon->min_hand_y && weapon_pose->offset_mm[1] <= weapon->max_hand_y;
    bool drawing = !strcmp(state, "WeaponEquipOn");
    bool attention = !strcmp(state, "WaitAttention");
    if (gear.id[SWMP_GEAR_BOW] && bow_pose) {
        const int16_t *bow = bow_pose->offset_mm;
        // The bow passes Link's left side on its way between the back and aiming pose.
        bool at_side = bow[0] >= 150 && bow[1] <= 1000;
        if (drawing && at_side) return SWMP_PROXY_BOW_DRAW;
        if (attention && !strcmp(upper, "WaitAttentionUpper")) {
            if (at_side) return SWMP_PROXY_BOW_STOW;
            // After release, WaitAttentionUpper holds the bow low and in front of Link.
            if (bow[1] >= 500 && bow[1] <= 900 && bow[2] >= 225) return SWMP_PROXY_BOW_READY;
        }
    }
    for (unsigned i = 0; i < SWMP_PROXY_ACTION_COUNT; i++) {
        const ProxyAction *action = &proxy_actions[i];
        if (!action->native) continue;
        const char *name = state;
        if (action->gear == PROXY_GEAR_BOW) {
            if (!gear.id[SWMP_GEAR_BOW]) continue;
            name = upper;
        } else if (action->gear == PROXY_GEAR_SHIELD) {
            if (!gear.id[SWMP_GEAR_SHIELD]) continue;
        } else if (action->gear && (action->gear != weapon->family || (drawing && !weapon_drawn))) continue;
        if (!strcmp(name, action->native)) return (SwmpProxyAction)i;
    }
    if (shield_raised && attention) return SWMP_PROXY_SHIELD_GUARD;
    if (weapon_drawn && (attention || !strcmp(state, "Wait"))) return weapon->ready;
    return SWMP_PROXY_IDLE;
}

static int compare_actions(const void *a, const void *b) {
    return strcmp((*(const ProxyAction *const *)a)->state, (*(const ProxyAction *const *)b)->state);
}

static void aamp_header(uint8_t *out, size_t end, unsigned lists, unsigned objects, unsigned params,
                        unsigned data_size, unsigned string_size) {
    memcpy(out, "AAMP", 4);
    write32(out + 4, 2);
    write32(out + 8, 3);  // little-endian, UTF-8
    write32(out + 12, (uint32_t)end);
    write32(out + 20, 4);  // four-byte "xml" type
    write32(out + 24, lists);
    write32(out + 28, objects);
    write32(out + 32, params);
    write32(out + 36, data_size);
    write32(out + 40, string_size);
    memcpy(out + 48, "xml", 4);
}

static uint32_t aamp_children(unsigned at, unsigned first, unsigned count) {
    return count ? (count << 16) | ((first - at) / 4) : 0;
}

static void aamp_list(uint8_t *out, unsigned at, const char *name, unsigned child, unsigned nchild,
                      unsigned object, unsigned nobject) {
    write32(out + at, crc(name));
    write32(out + at + 4, aamp_children(at, child, nchild));
    write32(out + at + 8, aamp_children(at, object, nobject));
}

static void aamp_object(uint8_t *out, unsigned at, const char *name, unsigned param, unsigned count) {
    write32(out + at, crc(name));
    write32(out + at + 4, aamp_children(at, param, count));
}

static void aamp_value(uint8_t *out, unsigned at, const char *key, unsigned type, unsigned value) {
    write32(out + at, crc(key));
    write32(out + at + 4, (type << 24) | ((value - at) / 4));
}

static unsigned aamp_string(uint8_t *out, unsigned at, const char *key, unsigned value, const char *text) {
    size_t length = strlen(text) + 1;
    aamp_value(out, at, key, 8, value);  // String64
    memcpy(out + value, text, length);
    return value + (unsigned)align4(length);
}

enum {
    PROXY_ANIM_LIST_SIZE = 124 + SWMP_PROXY_ACTION_COUNT * (24 + 2 * 64),
    PROXY_ANIM_STATE_SIZE = 120 + 64,
};

static size_t proxy_anim_list(uint8_t out[PROXY_ANIM_LIST_SIZE]) {
    enum { ROOT = 52, ADD_RES = ROOT + 12, DEFINES = ADD_RES + 12,
           RES_OBJECT = DEFINES + 12, FIRST_STATE = RES_OBJECT + 8,
           PARAMS = FIRST_STATE + 8 * SWMP_PROXY_ACTION_COUNT,
           STRINGS_AT = PARAMS + 8 * (1 + 2 * SWMP_PROXY_ACTION_COUNT) };
    aamp_list(out, ROOT, "param_root", ADD_RES, 2, 0, 0);
    aamp_list(out, ADD_RES, "AddReses", 0, 0, RES_OBJECT, 1);
    aamp_list(out, DEFINES, "ASDefines", 0, 0, FIRST_STATE, SWMP_PROXY_ACTION_COUNT);
    aamp_object(out, RES_OBJECT, "AddRes_0", PARAMS, 1);
    unsigned end = aamp_string(out, PARAMS, "Anim", STRINGS_AT, "Player_Animation");
    const ProxyAction *order[SWMP_PROXY_ACTION_COUNT];
    for (unsigned i = 0; i < SWMP_PROXY_ACTION_COUNT; i++) order[i] = &proxy_actions[i];
    qsort(order, SWMP_PROXY_ACTION_COUNT, sizeof *order, compare_actions);
    for (unsigned i = 0; i < SWMP_PROXY_ACTION_COUNT; i++) {
        char entry[24];
        snprintf(entry, sizeof entry, "ASDefine_%u", i);
        unsigned param = PARAMS + 8 + 16 * i;
        aamp_object(out, FIRST_STATE + 8 * i, entry, param, 2);
        end = aamp_string(out, param, "Name", end, order[i]->state);
        end = aamp_string(out, param + 8, "Filename", end, order[i]->file);
    }
    aamp_header(out, end, 3, 1 + SWMP_PROXY_ACTION_COUNT, 1 + 2 * SWMP_PROXY_ACTION_COUNT,
                0, end - STRINGS_AT);
    return end;
}

static size_t proxy_anim_state(uint8_t out[PROXY_ANIM_STATE_SIZE], const char *clip) {
    enum { ROOT = 52, ELEMENTS = ROOT + 12, CLIP = ELEMENTS + 12,
           PARAM_OBJECT = CLIP + 12, PARAMS = PARAM_OBJECT + 8, TYPE = PARAMS + 16,
           STRINGS_AT = TYPE + 4 };
    aamp_list(out, ROOT, "param_root", ELEMENTS, 1, 0, 0);
    aamp_list(out, ELEMENTS, "Elements", CLIP, 1, 0, 0);
    aamp_list(out, CLIP, "Element0", 0, 0, PARAM_OBJECT, 1);
    aamp_object(out, PARAM_OBJECT, "Parameters", PARAMS, 2);
    aamp_value(out, PARAMS, "TypeIndex", 2, TYPE);
    write32(out + TYPE, 67);  // skeletal clip
    unsigned end = aamp_string(out, PARAMS + 8, "FileName", STRINGS_AT, clip);
    aamp_header(out, end, 3, 1, 2, 4, end - STRINGS_AT);
    return end;
}

static uint8_t *proxy_actor_link(uint8_t *src, size_t size, const char *key, const char *value, size_t *out_size) {
    if (size < 52 || size > 4096 || (size & 3) || read32(src + 12) != size ||
        read32(src + 44) || !set_dummy_physics(src, size)) return NULL;
    uint8_t *param = aamp_param(src, size, key);
    if (!param) return NULL;
    size_t text_size = strlen(value) + 1;
    *out_size = size + align4(text_size);
    uint8_t *out = calloc(1, *out_size);
    if (!out) return NULL;
    memcpy(out, src, size);
    memcpy(out + size, value, text_size);
    aamp_value(out, (unsigned)(param - src), key, 20, (unsigned)size);
    write32(out + 12, (uint32_t)*out_size);
    write32(out + 40, read32(src + 40) + (uint32_t)(*out_size - size));
    return out;
}

uint8_t *swmp_proxy_pack(const uint8_t *src, size_t size, size_t *out_size) {
    static const char stock_link[] = "Actor/ActorLink/Obj_RockGray_A_M_01.bxml";
    static const char proxy_link[] = "Actor/ActorLink/" SWMP_PROXY_ACTOR ".bxml";
    static const char model_list[] = "Actor/ModelList/Obj_RockGray_A_M_01.bmodellist";
    size_t n;
    uint8_t *raw = src ? yaz0_unpack(src, size, &n) : NULL;
    if (!raw) return NULL;
    uint32_t data, ls, le, ms, me;
    uint8_t *out = NULL;
    if (sarc_find(raw, n, stock_link, &data, &ls, &le) >= 0 &&
        sarc_find(raw, n, model_list, &data, &ms, &me) >= 0 &&
        replace_model_name(raw + data + ms, me - ms, model_archive) &&
        replace_model_name(raw + data + ms, me - ms, template_actor)) {
        size_t link_size;
        uint8_t *link = proxy_actor_link(raw + data + ls, le - ls, "ASUser", SWMP_PROXY_ACTOR, &link_size);
        if (link) {
            uint8_t list[PROXY_ANIM_LIST_SIZE] = {0}, clips[SWMP_PROXY_ACTION_COUNT][PROXY_ANIM_STATE_SIZE] = {{0}};
            PackFile files[3 + SWMP_PROXY_ACTION_COUNT] = {
                {proxy_link, link, link_size},
                {model_list, raw + data + ms, me - ms},
                {"Actor/ASList/" SWMP_PROXY_ACTOR ".baslist", list, proxy_anim_list(list)},
            };
            for (unsigned i = 0; i < SWMP_PROXY_ACTION_COUNT; i++)
                files[3 + i] = (PackFile){proxy_actions[i].path, clips[i], proxy_anim_state(clips[i], proxy_actions[i].clip)};
            out = pack_files(files, sizeof files / sizeof files[0], out_size);
            free(link);
        }
    }
    free(raw);
    return out;
}

uint8_t *swmp_gear_pack(const SwmpGearAsset *asset, const uint8_t *src, size_t size,
                        const uint8_t *gear, size_t gear_size, size_t *out_size) {
    static const char stock_link[] = "Actor/ActorLink/Obj_RockGray_A_M_01.bxml";
    static const char *const glider_anim[] = {
        "Actor/ASList/PlayerStole.baslist", "Actor/AS/PlayerStole_Open.bas",
    };
    static const char *const bow_anim[] = {
        "Actor/ASList/Bow.baslist", "Actor/AS/Bow_Draw.bas", "Actor/AS/Bow_Equiped.bas",
        "Actor/AS/Bow_Hold.bas", "Actor/AS/Bow_Null.bas", "Actor/AS/Bow_SelfCameraPause.bas",
        "Actor/AS/Bow_Shoot.bas",
    };
    size_t n, gn;
    uint8_t *raw = src ? yaz0_unpack(src, size, &n) : NULL;
    uint8_t *model = gear ? yaz0_unpack(gear, gear_size, &gn) : NULL;
    uint8_t *out = NULL;
    uint32_t data, ls, le, model_data, ms, me;
    // Keep the prop profile and dummy physics so the visual stays out of pickup, inventory and hit processing.
    if (raw && model && sarc_find(raw, n, stock_link, &data, &ls, &le) >= 0 &&
        sarc_find(model, gn, asset->model_list, &model_data, &ms, &me) >= 0) {
        size_t link_size;
        uint8_t *link = proxy_actor_link(raw + data + ls, le - ls, "ModelUser", asset->model_user, &link_size);
        if (link) {
            PackFile files[2 + sizeof bow_anim / sizeof *bow_anim] = {
                {asset->actor_link, link, link_size},
                {asset->model_list, model + model_data + ms, me - ms},
            };
            unsigned count = 2;
            char alias_list[80];
            bool glider = asset->slot == SWMP_PARAGLIDER_SLOT;
            const char *const *anim = glider ? glider_anim : bow_anim;
            unsigned clips = glider ? sizeof glider_anim / sizeof *glider_anim :
                asset->slot == SWMP_GEAR_BOW ? sizeof bow_anim / sizeof *bow_anim : 0;
            if (clips) {
                snprintf(alias_list, sizeof alias_list, "Actor/ASList/%s.baslist", asset->actor);
                for (unsigned i = 0; i < clips; i++) {
                    uint32_t start, end;
                    if (sarc_find(model, gn, anim[i], &model_data, &start, &end) < 0) break;
                    files[count++] = (PackFile){i ? anim[i] : alias_list, model + model_data + start, end - start};
                }
                uint8_t *animated = count == 2 + clips ?
                    proxy_actor_link(link, link_size, "ASUser", asset->actor, &link_size) : NULL;
                if (animated) {
                    free(link);
                    link = animated;
                    files[0] = (PackFile){asset->actor_link, link, link_size};
                } else {
                    count = 2;
                }
            }
            if (!glider || count > 2) out = pack_files(files, count, out_size);
            free(link);
        }
    }
    free(model);
    free(raw);
    return out;
}
#endif
