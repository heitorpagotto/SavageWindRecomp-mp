#include "multiplayer_assets.h"

#ifdef SW_MULTIPLAYER
#include "services/archive.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

static const char template_actor[] = "Obj_RockGray_A_M_01";
static const char link_model[] = "Link";
static const char link_elink[] = "Player_GameRom";

#define VISUAL(slot, source, model, actor, label, bundle) {slot, model, actor, label, \
    "Actor/Pack/" source ".sbactorpack", bundle, "Actor/Pack/" actor ".sbactorpack", \
    "Actor/ActorLink/" actor ".bxml", source, "Actor/ModelList/" source ".bmodellist"}
#define ARROW(type, source, model, actor, label) \
    [SWMP_ARROW_ASSET_BASE + SWMP_ARROW_##type] = VISUAL(SWMP_NOCKED_ARROW_SLOT, source, model, actor, label, "Pack/TitleBG.pack")
const SwmpGearAsset swmp_gear_assets[SWMP_VISUAL_ASSET_COUNT] = {
    [SWMP_PARAGLIDER_ASSET] = VISUAL(SWMP_PARAGLIDER_SLOT, "PlayerStole2", "Item_Parastole2",
                                    "SwmpRemoteGlider", "glider", "Pack/TitleBG.pack"),
    ARROW(NORMAL, "NormalArrow", "Weapon_Arrow_001", "SwmpRemoteArrow", "nocked arrow"),
    ARROW(FIRE, "FireArrow", "Weapon_Arrow_005", "SwmpRemoteFireArrow", "nocked fire arrow"),
    ARROW(ICE, "IceArrow", "Weapon_Arrow_006", "SwmpRemoteIceArrow", "nocked ice arrow"),
    ARROW(SHOCK, "ElectricArrow", "Weapon_Arrow_004", "SwmpRemoteShockArrow", "nocked shock arrow"),
    ARROW(BOMB, "BombArrow_A", "Weapon_Arrow_002", "SwmpRemoteBombArrow", "nocked bomb arrow"),
    ARROW(ANCIENT, "AncientArrow", "Weapon_Arrow_007", "SwmpRemoteAncientArrow", "nocked ancient arrow"),
    [SWMP_HORSE_ASSET] = VISUAL(SWMP_HORSE_SLOT, "GameRomHorse", "Horse_Link", "SwmpRemoteHorse", "horse", NULL),
};
#undef ARROW
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

static bool remap_string_refs(const Byml *b, uint32_t offset, unsigned type, const uint32_t *indices, uint32_t count,
                              uint8_t *visited, unsigned depth) {
    BymlNode n = node(b, offset, type);
    if (!n.head || depth == 16) return false;
    if (visited[offset / 4]) return true;
    visited[offset / 4] = 1;
    for (uint32_t i = 0; i < n.count; i++) {
        uint8_t *value = n.values + i * (type == ARRAY ? 4 : 8) + (type == DICT ? 4 : 0);
        unsigned tag = type == ARRAY ? n.head[4 + i] : value[-1];
        uint32_t ref = read32(value);
        if (tag == STRING) {
            if (ref >= count) return false;
            write32(value, indices[ref]);
        }
        if ((tag == ARRAY || tag == DICT) &&
            !remap_string_refs(b, ref, tag, indices, count, visited, depth + 1))
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

typedef struct {
    BymlNode keys, strings, root, actors, hashes;
    uint8_t *actors_ref, *hashes_ref;
} ActorTable;

static bool actor_table(const Byml *b, ActorTable *t) {
    if (b->original_size < 16 || memcmp(b->data, "YB\x02\0", 4)) return false;
    t->keys = node(b, read32(b->data + 4), STRINGS);
    t->strings = node(b, read32(b->data + 8), STRINGS);
    t->root = node(b, read32(b->data + 12), DICT);
    t->actors_ref = field(b, t->keys, t->root, "Actors", ARRAY);
    t->hashes_ref = field(b, t->keys, t->root, "Hashes", ARRAY);
    if (!t->actors_ref || !t->hashes_ref || !t->strings.count) return false;
    t->actors = node(b, read32(t->actors_ref), ARRAY);
    t->hashes = node(b, read32(t->hashes_ref), ARRAY);
    return t->actors.count && t->actors.count == t->hashes.count;
}

static BymlNode actor_entry(const Byml *b, const ActorTable *t, unsigned index) {
    return t->actors.head[4 + index] == DICT ?
        node(b, read32(t->actors.values + 4 * index), DICT) : (BymlNode){0};
}

static const char *actor_string(const Byml *b, const ActorTable *t, BymlNode actor, const char *key) {
    const uint8_t *ref = field(b, t->keys, actor, key, STRING);
    return ref ? string_at(b, t->strings, read32(ref)) : NULL;
}

static const struct {
    const char *prefix;
    const char *profile;
    SwmpGearSlot slot;
} gear_families[SWMP_GEAR_FAMILY_COUNT] = {
    [SWMP_GEAR_FAMILY_SWORD] = {"Weapon_Sword_", "WeaponSmallSword", SWMP_GEAR_WEAPON},
    [SWMP_GEAR_FAMILY_LSWORD] = {"Weapon_Lsword_", "WeaponLargeSword", SWMP_GEAR_WEAPON},
    [SWMP_GEAR_FAMILY_SPEAR] = {"Weapon_Spear_", "WeaponSpear", SWMP_GEAR_WEAPON},
    [SWMP_GEAR_FAMILY_SHIELD] = {"Weapon_Shield_", "WeaponShield", SWMP_GEAR_SHIELD},
    [SWMP_GEAR_FAMILY_BOW] = {"Weapon_Bow_", "WeaponBow", SWMP_GEAR_BOW},
};

static uint16_t gear_identity(const char *name) {
    if (!name || strncmp(name, "Weapon_", 7)) return SWMP_GEAR_NONE;
    for (unsigned i = 1; i < SWMP_GEAR_FAMILY_COUNT; i++) {
        const char *prefix = gear_families[i].prefix;
        size_t length = strlen(prefix);
        if (strncmp(name, prefix, length)) continue;
        const char *suffix = name + length;
        if (strspn(suffix, "0123456789") != 3 || suffix[3]) return SWMP_GEAR_NONE;
        unsigned variant = (suffix[0] - '0') * 100 + (suffix[1] - '0') * 10 + suffix[2] - '0';
        return variant ? (uint16_t)(i * SWMP_GEAR_VARIANTS + variant) : SWMP_GEAR_NONE;
    }
    return SWMP_GEAR_NONE;
}

uint16_t swmp_gear_identity(SwmpGearSlot slot, const char *name) {
    uint16_t id = gear_identity(name);
    return id && gear_families[swmp_gear_family(id)].slot == slot ? id : SWMP_GEAR_NONE;
}

static bool resource_name(const char *name, size_t capacity) {
    return name && *name && strlen(name) < capacity &&
        name[strspn(name, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_")] == 0;
}

static int compare_gear_models(const void *a, const void *b) {
    uint32_t x = ((const SwmpGearModel *)a)->id, y = ((const SwmpGearModel *)b)->id;
    return x < y ? -1 : x > y;
}

uint32_t swmp_item_identity(const char *name) {
    if (!resource_name(name, sizeof ((SwmpGearModel *)0)->name)) return 0;
    uint16_t gear = gear_identity(name);
    return gear ? gear : crc(name) | UINT32_C(0x80000000);
}

bool swmp_gear_catalog(const uint8_t *src, size_t size, SwmpGearCatalog *out) {
    size_t n;
    uint8_t *raw = src ? yaz0_unpack(src, size, &n) : NULL;
    out->count = 0;
    if (!raw) return false;
    Byml b = {.data = raw, .original_size = n};
    ActorTable t;
    if (n > 16 * 1024 * 1024 || !actor_table(&b, &t)) {
        free(raw);
        return false;
    }
    for (unsigned i = 0; i < t.actors.count && out->count < SWMP_MAX_GEAR_MODELS; i++) {
        BymlNode actor = actor_entry(&b, &t, i);
        const char *name = actor_string(&b, &t, actor, "name");
        const char *profile = actor_string(&b, &t, actor, "profile");
        if (!profile || !resource_name(name, sizeof out->models[0].name)) continue;
        uint16_t gear = gear_identity(name);
        bool prop = !strncmp(name, "Obj_Lift", 8) || !strncmp(name, "Obj_Barrel", 10) ||
            !strncmp(name, "Obj_Pot", 7);
        bool bomb = !strncmp(name, "RemoteBomb", 10) && !strcmp(profile, "PlayerItem");
        bool material = !strcmp(profile, "Item") || !strcmp(profile, "CapturedActor");
        if (gear ? strcmp(profile, gear_families[swmp_gear_family(gear)].profile) :
            !material && !prop && !bomb) continue;
        const char *folder = actor_string(&b, &t, actor, "bfres");
        const char *model = actor_string(&b, &t, actor, "mainModel");
        SwmpGearModel *asset = &out->models[out->count];
        // The native model list can describe multiple units without a mainModel entry.
        if (!model) model = folder;
        if (!resource_name(folder, sizeof asset->folder) || !resource_name(model, sizeof asset->model)) continue;
        *asset = (SwmpGearModel){.id = swmp_item_identity(name)};
        snprintf(asset->actor, sizeof asset->actor, "SwmpGear%04x", (unsigned)asset->id);
        memcpy(asset->name, name, strlen(name) + 1);
        memcpy(asset->folder, folder, strlen(folder) + 1);
        memcpy(asset->model, model, strlen(model) + 1);
        out->count++;
    }
    qsort(out->models, out->count, sizeof out->models[0], compare_gear_models);
    for (unsigned i = 1; i < out->count; i++) {
        if (out->models[i-1].id != out->models[i].id) continue;
        // An ambiguous installed actor table must not give one identity two visuals.
        out->count = 0;
        free(raw);
        return false;
    }
    free(raw);
    return true;
}

static const char *const default_armor_names[SWMP_ARMOR_SLOTS] = {
    "Armor_Default_Head", "Armor_Default_Upper", "Armor_Default_Lower",
};

static bool armor_resource_name(const char *name, size_t capacity) {
    return name && !strncmp(name, "Armor_", 6) && resource_name(name, capacity);
}

bool swmp_armor_catalog(const uint8_t *src, size_t size, SwmpArmorCatalog *out) {
    size_t n;
    uint8_t *raw = src ? yaz0_unpack(src, size, &n) : NULL;
    *out = (SwmpArmorCatalog){0};
    if (!raw) return false;
    Byml b = {.data = raw, .original_size = n};
    ActorTable t;
    if (n > 16 * 1024 * 1024 || !actor_table(&b, &t)) {
        free(raw);
        return false;
    }
    static const char *const profiles[SWMP_ARMOR_SLOTS] = {"ArmorHead", "ArmorUpper", "ArmorLower"};
    for (unsigned i = 0; i < t.actors.count; i++) {
        BymlNode actor = actor_entry(&b, &t, i);
        const char *profile = actor_string(&b, &t, actor, "profile");
        if (!profile) continue;
        SwmpArmorSlot slot = 0;
        while (slot < SWMP_ARMOR_SLOTS && strcmp(profile, profiles[slot])) slot++;
        if (slot == SWMP_ARMOR_SLOTS) continue;
        const char *name = actor_string(&b, &t, actor, "name");
        const char *folder = actor_string(&b, &t, actor, "bfres");
        const char *model = actor_string(&b, &t, actor, "mainModel");
        if (!armor_resource_name(name, 48) || !armor_resource_name(folder, sizeof out->models[0].folder) ||
            !armor_resource_name(model, sizeof out->models[0].model) || out->name_count == SWMP_MAX_ARMOR_NAMES) continue;
        uint32_t id = crc(model);
        if (!id) continue;
        unsigned index = 0;
        while (index < out->model_count && out->models[index].id != id) index++;
        SwmpArmorAsset *asset = &out->models[index < SWMP_MAX_ARMOR_MODELS ? index : 0];
        if (index == out->model_count) {
            if (index == SWMP_MAX_ARMOR_MODELS) continue;
            const uint8_t *heap = field(&b, t.keys, actor, "instSize", INT);
            uint32_t native_size = heap ? read32(heap) : 0;
            *asset = (SwmpArmorAsset){.id = id, .slot = slot,
                .heap_size = 512 * 1024 + (native_size <= 1024 * 1024 ? native_size : 0)};
            snprintf(asset->actor, sizeof asset->actor, SWMP_ARMOR_ACTOR_PREFIX "%08x", id);
            memcpy(asset->folder, folder, strlen(folder) + 1);
            memcpy(asset->model, model, strlen(model) + 1);
            out->model_count++;
        } else if (asset->slot != slot || strcmp(asset->folder, folder) || strcmp(asset->model, model)) {
            continue;
        }
        if (!strcmp(name, default_armor_names[slot])) out->defaults[slot] = id;
        unsigned binding = out->name_count++;
        out->names[binding].native_id = crc(name);
        out->names[binding].model_id = id;
        out->names[binding].slot = slot;
    }
    free(raw);
    return true;
}

uint32_t swmp_armor_identity(const SwmpArmorCatalog *catalog, SwmpArmorSlot slot, const char *native_name) {
    uint32_t id = crc(native_name);
    for (unsigned i = 0; i < catalog->name_count; i++) {
        if (catalog->names[i].native_id != id || catalog->names[i].slot != slot) continue;
        uint32_t model = catalog->names[i].model_id;
        return model == catalog->defaults[slot] ? 0 : model;
    }
    return 0;
}

typedef struct {
    const char *name, *folder, *model;
    uint32_t heap_size, hash, name_index, actor_offset;
} ActorAlias;

static int alias_name_order(const void *a, const void *b) {
    return strcmp(((const ActorAlias *)a)->name, ((const ActorAlias *)b)->name);
}

static int alias_hash_order(const void *a, const void *b) {
    uint32_t x = ((const ActorAlias *)a)->hash, y = ((const ActorAlias *)b)->hash;
    return (x > y) - (x < y);
}

static uint32_t lookup_string(const Byml *b, BymlNode table, const char *name) {
    uint32_t lo = 0, hi = table.count;
    while (lo < hi) {
        uint32_t mid = lo + (hi - lo) / 2;
        const char *s = string_at(b, table, mid);
        if (!s) return UINT32_MAX;
        int order = strcmp(s, name);
        if (!order) return mid;
        if (order < 0) lo = mid + 1;
        else hi = mid;
    }
    return UINT32_MAX;
}

static void set_alias_elink(uint8_t *actor, uint32_t count, uint32_t key, uint32_t user) {
    // ActorInfo dictionaries are ordered by key index.
    uint8_t *entry = actor + 4, *end = entry + 8 * count;
    while (entry < end && (read32(entry) & 0xffffff) < key) entry += 8;
    if (entry == end || (read32(entry) & 0xffffff) != key) {
        memmove(entry + 8, entry, (size_t)(end - entry));
        write32(actor, ((count + 1) << 8) | DICT);
    }
    write32(entry, ((uint32_t)STRING << 24) | key);
    write32(entry + 4, user);
}

static bool add_actor_aliases(Byml *b, ActorAlias *alias, unsigned count) {
    ActorTable t;
    if (!actor_table(b, &t)) return false;
    BymlNode stock = {0};
    uint32_t stock_hash = crc(template_actor);
    for (unsigned i = 0; i < t.hashes.count; i++)
        if (read32(t.hashes.values + i * 4) == stock_hash) stock = actor_entry(b, &t, i);
    const char *stock_name = actor_string(b, &t, stock, "name");
    uint8_t *name = field(b, t.keys, stock, "name", STRING);
    uint8_t *folder = field(b, t.keys, stock, "bfres", STRING);
    uint8_t *model = field(b, t.keys, stock, "mainModel", STRING);
    uint8_t *heap = field(b, t.keys, stock, "instSize", INT);
    if (!stock_name || strcmp(stock_name, template_actor) || !folder || !model || !heap) return false;

    uint32_t *indices = malloc(t.strings.count * sizeof *indices);
    uint8_t *visited = calloc(b->original_size / 4 + 1, 1);
    if (!indices || !visited) {
        free(indices);
        free(visited);
        return false;
    }
    qsort(alias, count, sizeof *alias, alias_name_order);
    size_t extra = 4 * count;
    for (unsigned i = 0; i < count; i++) {
        extra += strlen(alias[i].name) + 1;
        alias[i].hash = crc(alias[i].name);
    }
    uint32_t strings_end = read32(t.strings.values + 4 * t.strings.count);
    uint8_t *strings = strings_end <= b->original_size ? append(b, strings_end + extra) : NULL;
    bool ok = strings != NULL;
    if (ok) {
        unsigned n = t.strings.count + count, offset = 8 + 4 * n;
        write32(strings, (n << 8) | STRINGS);
        unsigned first = 0, added = 0;
        for (unsigned i = 0; ok && i < n; i++) {
            const char *s = first < t.strings.count ? string_at(b, t.strings, first) : NULL;
            if (first < t.strings.count && !s) { ok = false; break; }
            int order = !s ? 1 : added == count ? -1 : strcmp(s, alias[added].name);
            if (!order) { ok = false; break; }
            if (order < 0) indices[first++] = i;
            else {
                alias[added].name_index = i;
                s = alias[added++].name;
            }
            size_t length = strlen(s) + 1;
            if (offset + length > strings_end + extra) { ok = false; break; }
            write32(strings + 4 + 4 * i, offset);
            memcpy(strings + offset, s, length);
            offset += (unsigned)length;
        }
        write32(strings + 4 + 4 * n, offset);
    }
    if (ok) ok = remap_string_refs(b, read32(b->data + 12), DICT, indices, t.strings.count, visited, 0);
    free(visited);
    uint32_t elink_key = lookup_string(b, t.keys, "elink");
    uint32_t elink_user = lookup_string(b, t.strings, link_elink);
    for (unsigned i = 0; ok && i < count; i++) {
        uint32_t folder_index = lookup_string(b, t.strings, alias[i].folder);
        uint32_t model_index = lookup_string(b, t.strings, alias[i].model);
        size_t actor_size = 4 + 8 * stock.count;
        // Native XLink creates its ELink instance using ActorInfo's user name.
        bool link_effect = elink_key != UINT32_MAX && elink_user != UINT32_MAX &&
            !strcmp(alias[i].name, SWMP_PROXY_ACTOR);
        uint8_t *actor = append(b, actor_size + (link_effect ? 8 : 0));
        if (!actor || folder_index == UINT32_MAX || model_index == UINT32_MAX) { ok = false; break; }
        memcpy(actor, stock.head, actor_size);
        write32(actor + (name - stock.head), alias[i].name_index);
        write32(actor + (folder - stock.head), indices[folder_index]);
        write32(actor + (model - stock.head), indices[model_index]);
        write32(actor + (heap - stock.head), alias[i].heap_size);
        if (link_effect) set_alias_elink(actor, stock.count, elink_key, indices[elink_user]);
        alias[i].actor_offset = (uint32_t)(actor - b->data);
    }
    free(indices);
    if (!ok) return false;

    unsigned n = t.actors.count + count;
    size_t prefix = 4 + align4(n), root_size = 4 + 8 * t.root.count;
    uint8_t *actors = append(b, prefix + 4 * n), *hashes = append(b, prefix + 4 * n);
    uint8_t *root = append(b, root_size);
    if (!actors || !hashes || !root) return false;
    qsort(alias, count, sizeof *alias, alias_hash_order);
    write32(actors, (n << 8) | ARRAY);
    write32(hashes, (n << 8) | ARRAY);
    unsigned first = 0, added = 0;
    for (unsigned i = 0; i < n; i++) {
        uint32_t hash = first < t.hashes.count ? read32(t.hashes.values + 4 * first) : 0;
        if (added < count && first < t.actors.count && hash == alias[added].hash) return false;
        if (first == t.actors.count || (added < count && alias[added].hash < hash)) {
            ActorAlias *a = &alias[added++];
            if (added < count && a->hash == alias[added].hash) return false;
            actors[4 + i] = DICT;
            hashes[4 + i] = a->hash > INT32_MAX ? UINT : INT;
            write32(actors + prefix + 4 * i, a->actor_offset);
            write32(hashes + prefix + 4 * i, a->hash);
        } else {
            actors[4 + i] = t.actors.head[4 + first];
            hashes[4 + i] = t.hashes.head[4 + first];
            write32(actors + prefix + 4 * i, read32(t.actors.values + 4 * first++));
            write32(hashes + prefix + 4 * i, hash);
        }
    }
    memcpy(root, t.root.head, root_size);
    write32(root + (t.actors_ref - t.root.head), (uint32_t)(actors - b->data));
    write32(root + (t.hashes_ref - t.root.head), (uint32_t)(hashes - b->data));
    write32(b->data + 8, (uint32_t)(strings - b->data));
    write32(b->data + 12, (uint32_t)(root - b->data));
    return true;
}

uint8_t *swmp_proxy_actor_info(const uint8_t *src, size_t size, unsigned gear_mask,
                              const SwmpGearCatalog *gear, const SwmpArmorCatalog *armor, size_t *out_size) {
    size_t n;
    uint8_t *raw = src ? yaz0_unpack(src, size, &n) : NULL;
    if (!raw || n < 16 || n > 16 * 1024 * 1024) {
        free(raw);
        return NULL;
    }
    ActorAlias aliases[SWMP_VISUAL_ASSET_COUNT + SWMP_MAX_GEAR_MODELS + SWMP_MAX_ARMOR_MODELS] = {
        {.name = SWMP_PROXY_ACTOR, .folder = link_model, .model = link_model, .heap_size = 2 * 1024 * 1024},
    };
    unsigned count = 1;
    for (unsigned i = 1; i < SWMP_VISUAL_ASSET_COUNT; i++) {
        if (!(gear_mask & (1u << i))) continue;
        const SwmpGearAsset *a = &swmp_gear_assets[i];
        aliases[count++] = (ActorAlias){.name = a->actor, .folder = a->model, .model = a->model,
                                        .heap_size = 256 * 1024};
    }
    for (unsigned i = 0; gear && i < gear->count; i++) {
        const SwmpGearModel *a = &gear->models[i];
        if (a->ready) aliases[count++] = (ActorAlias){.name = a->actor, .folder = a->folder,
            .model = a->model, .heap_size = 256 * 1024};
    }
    for (unsigned i = 0; armor && i < armor->model_count; i++) {
        const SwmpArmorAsset *a = &armor->models[i];
        if (a->ready) aliases[count++] = (ActorAlias){.name = a->actor, .folder = a->folder,
            .model = a->model, .heap_size = a->heap_size};
    }
    // Merge aliases into each table once so the archive size grows with the number of models.
    Byml b = {.original_size = n, .length = n, .capacity = 3 * n + 2048 * count};
    b.data = calloc(1, b.capacity);
    if (b.data) memcpy(b.data, raw, n);
    free(raw);
    if (!b.data || !add_actor_aliases(&b, aliases, count)) {
        free(b.data);
        return NULL;
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

static uint8_t *aamp_param(uint8_t *data, size_t size, const char *key, uint8_t type) {
    uint32_t hash = crc(key);
    if (size < 8 || memcmp(data, "AAMP", 4)) return NULL;
    for (size_t i = 0; i <= size - 8; i += 4)
        if (read32(data + i) == hash && data[i + 7] == type) return data + i;
    return NULL;
}

static bool set_dummy_physics(uint8_t *data, size_t size) {
    uint8_t *ai = aamp_param(data, size, "AIProgramUser", 20);
    uint8_t *physics = aamp_param(data, size, "PhysicsUser", 20);
    if (!ai || !physics) return false;
    size_t dummy = (size_t)(ai - data) + (read32(ai + 4) & 0xffffff) * 4;
    if (dummy + sizeof "Dummy" > size || memcmp(data + dummy, "Dummy", sizeof "Dummy") ||
        data + dummy <= physics) return false;
    // AAMP string parameters encode the type and a word offset from the parameter entry.
    write32(physics + 4, (20u << 24) | (uint32_t)((data + dummy - physics) / 4));
    return true;
}

typedef enum {
    PROXY_GEAR_NONE = SWMP_GEAR_FAMILY_NONE,
    PROXY_GEAR_LSWORD = SWMP_GEAR_FAMILY_LSWORD,
    PROXY_GEAR_SWORD = SWMP_GEAR_FAMILY_SWORD,
    PROXY_GEAR_SPEAR = SWMP_GEAR_FAMILY_SPEAR,
    PROXY_GEAR_BOW = SWMP_GEAR_FAMILY_BOW,
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
    [SWMP_PROXY_AXE_DRAW] = ACTION(AxeDraw, "Equip_Lsword_On_Natural", NULL, PROXY_GEAR_NONE),
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
    [SWMP_PROXY_SWORD_DRAW] = ACTION(SwordDraw, "Equip_Sword_On_Natural", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_SWORD_READY] = ACTION(SwordReady, "A_Pose_Sword_Wait", NULL, PROXY_GEAR_SWORD),
    [SWMP_PROXY_SWORD_STOW] = ACTION(SwordStow, "Equip_Sword_Off_Natural", "WeaponEquipOff", PROXY_GEAR_SWORD),
    [SWMP_PROXY_SWORD_SWING] = ACTION(SwordSwing, "Sword_Attack_S1", "CutS1", PROXY_GEAR_SWORD),
    [SWMP_PROXY_SWORD_CHAIN_2] = ACTION(SwordChain2, "Sword_Attack_S2", "CutS2", PROXY_GEAR_SWORD),
    [SWMP_PROXY_SWORD_CHAIN_3] = ACTION(SwordChain3, "Sword_Attack_S3", "CutS3", PROXY_GEAR_SWORD),
    [SWMP_PROXY_SWORD_FINISH] = ACTION(SwordFinish, "Sword_Attack_SF", "CutSF", PROXY_GEAR_SWORD),
    [SWMP_PROXY_SPEAR_DRAW] = ACTION(SpearDraw, "Equip_Spear_On_Natural", NULL, PROXY_GEAR_NONE),
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
    [SWMP_PROXY_SHIELD_PARRY] = ACTION(ShieldParry, "Shield_Guard_Just", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_SHIELD_HIT] = ACTION(ShieldHit, "Shield_Guard_Hit", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_SHIELD_BREAK] = ACTION(ShieldBreak, "Shield_Guard_Break", NULL, PROXY_GEAR_NONE),
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
    [SWMP_PROXY_CLIMB_RELEASE] = ACTION(ClimbRelease, "Climb_WallOff_ToWait_FootL", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_SHIELD_WALK_FORWARD] = ACTION(ShieldWalkForward, "Nml_Lockon_Walk_F", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_SHIELD_WALK_BACK] = ACTION(ShieldWalkBack, "Nml_Lockon_Walk_B", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_SHIELD_WALK_LEFT] = ACTION(ShieldWalkLeft, "Nml_Lockon_Walk_L", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_SHIELD_WALK_RIGHT] = ACTION(ShieldWalkRight, "Nml_Lockon_Walk_R", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_SURF_MOUNT] = ACTION(SurfMount, "Shield_Board_ON", "SurfingOn", PROXY_GEAR_NONE),
    [SWMP_PROXY_SURF_MOUNT_GLIDE] = ACTION(SurfMountGlide, "Shield_Board_ON_Para", "SurfingOnPara", PROXY_GEAR_NONE),
    [SWMP_PROXY_SURF_RIDE] = ACTION(SurfRide, "Shield_Board_Go2", "SurfingSlide", PROXY_GEAR_NONE),
    [SWMP_PROXY_SURF_JUMP] = ACTION(SurfJump, "Shield_Board_Jump", "SurfingJump", PROXY_GEAR_NONE),
    [SWMP_PROXY_SURF_AIR] = ACTION(SurfAir, "Shield_Board_Fall_Loop", "SurfingAir", PROXY_GEAR_NONE),
    [SWMP_PROXY_SURF_LAND] = ACTION(SurfLand, "Shield_Board_Land", "SurfingLand", PROXY_GEAR_NONE),
    [SWMP_PROXY_SURF_DISMOUNT] = ACTION(SurfDismount, "Shield_Board_OFF", "SurfingOff", PROXY_GEAR_NONE),
    [SWMP_PROXY_SURF_STAGGER] = ACTION(SurfStagger, "Shield_Board_Stagger_Slope_0", "SurfingStagger", PROXY_GEAR_NONE),
    [SWMP_PROXY_SURF_FAILED] = ACTION(SurfFailed, "Shield_Board_Go_NG", "SurfingNG", PROXY_GEAR_NONE),
    [SWMP_PROXY_SHIELD_DRAW] = ACTION(ShieldDraw, "Equip_Shield_On", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_SWORD_THROW_WINDUP] = ACTION(SwordThrowWindup, "Sword_Throw_Charge", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_LSWORD_THROW_WINDUP] = ACTION(LargeSwordThrowWindup, "Lsword_Throw_Charge", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_SPEAR_THROW_WINDUP] = ACTION(SpearThrowWindup, "Spear_Throw_Charge", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_SWORD_THROW_AIM] = ACTION(SwordThrowAim, "Sword_Throw_Charge_Wait", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_LSWORD_THROW_AIM] = ACTION(LargeSwordThrowAim, "Lsword_Throw_Charge_Wait", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_SPEAR_THROW_AIM] = ACTION(SpearThrowAim, "Spear_Throw_Charge_Wait", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_SWORD_THROW_RELEASE] = ACTION(SwordThrowRelease, "Sword_Throw_Throw", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_LSWORD_THROW_RELEASE] = ACTION(LargeSwordThrowRelease, "Lsword_Throw_Throw", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_SPEAR_THROW_RELEASE] = ACTION(SpearThrowRelease, "Spear_Throw_Throw", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_SWORD_THROW_CATCH] = ACTION(SwordThrowCatch, "Sword_Throw_Catch", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_LSWORD_THROW_CATCH] = ACTION(LargeSwordThrowCatch, "Lsword_Throw_Catch", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_HOLD_FEW] = ACTION(HoldFew, "Lift_Lift_Pouch_Few", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_HOLD_MANY] = ACTION(HoldMany, "Lift_Lift_Pouch_Many", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_PUT_MATERIALS] = ACTION(PutMaterials, "Lift_Put_Pouch_L", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_TOSS_MATERIALS] = ACTION(TossMaterials, "Lift_Put_Pouch_Far", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_LIFT_PROP] = ACTION(LiftProp, "Lift_Up_L", "GrabUp", PROXY_GEAR_NONE),
    [SWMP_PROXY_HOLD_PROP] = ACTION(HoldProp, "Lift_Up_L", "GrabWait", PROXY_GEAR_NONE),
    [SWMP_PROXY_PUT_PROP] = ACTION(PutProp, "Lift_Put_L", "GrabPut", PROXY_GEAR_NONE),
    [SWMP_PROXY_THROW_PROP] = ACTION(ThrowProp, "Lift_Throw_L", "GrabThrow", PROXY_GEAR_NONE),
    [SWMP_PROXY_WALK_HOLD_FEW] = ACTION(WalkHoldFew, "Lift_Lift_Pouch_Few", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_WALK_HOLD_MANY] = ACTION(WalkHoldMany, "Lift_Lift_Pouch_Many", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_RUN_HOLD_FEW] = ACTION(RunHoldFew, "Lift_Lift_Pouch_Few", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_RUN_HOLD_MANY] = ACTION(RunHoldMany, "Lift_Lift_Pouch_Many", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_LADDER_READY] = ACTION(LadderReady, "Ladder_Up_Ready_St", "LadderUpReadySt", PROXY_GEAR_NONE),
    [SWMP_PROXY_LADDER_WAIT] = ACTION(LadderWait, "Ladder_Wait_FootL", "LadderWait", PROXY_GEAR_NONE),
    [SWMP_PROXY_LADDER_UP] = ACTION(LadderUp, "Ladder_Up_FootL", "LadderUp", PROXY_GEAR_NONE),
    [SWMP_PROXY_LADDER_UP_RIGHT] = ACTION(LadderUpRight, "Ladder_Up_FootR", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_LADDER_UP_END] = ACTION(LadderUpEnd, "Ladder_Up_Ed_FootL", "LadderUpEd", PROXY_GEAR_NONE),
    [SWMP_PROXY_LADDER_DOWN_START] = ACTION(LadderDownStart, "Ladder_Down_St", "LadderDownSt", PROXY_GEAR_NONE),
    [SWMP_PROXY_LADDER_DOWN] = ACTION(LadderDown, "Ladder_Down", "LadderDown", PROXY_GEAR_NONE),
    [SWMP_PROXY_LADDER_DOWN_END] = ACTION(LadderDownEnd, "Ladder_Down_Ed_FootL", "LadderDownEd", PROXY_GEAR_NONE),
    [SWMP_PROXY_LADDER_JUMP] = ACTION(LadderJump, "Ladder_Jump_FootL", "LadderJump", PROXY_GEAR_NONE),
    [SWMP_PROXY_LADDER_LAND] = ACTION(LadderLand, "Ladder_DownToWait", "LadderLand", PROXY_GEAR_NONE),
    [SWMP_PROXY_POLE_MOUNT] = ACTION(PoleMount, "Climb_Tree_Ed_Sharp", "ClimbTreeEdSharp", PROXY_GEAR_NONE),
    [SWMP_PROXY_POLE_HOLD] = ACTION(PoleHold, "Climb_Tree_Top_Sharp_FootL", "ClimbTreeTopSharp", PROXY_GEAR_NONE),
    [SWMP_PROXY_WARP_DEPART] = ACTION(WarpDepart, "Demo_Warp_Nml", "Warp", PROXY_GEAR_NONE),
    [SWMP_PROXY_WARP_ARRIVE] = ACTION(WarpArrive, "Nml_Wait", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_HORSE_WAIT] = ACTION(RideWait, "Horse_Wait", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_HORSE_WALK] = ACTION(RideWalk, "Horse_Move_Gear_1", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_HORSE_TROT] = ACTION(RideTrot, "Horse_Move_Gear_2", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_HORSE_CANTER] = ACTION(RideCanter, "Horse_Move_Gear_3", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_HORSE_GALLOP] = ACTION(RideGallop, "Horse_Move_Gear_Top", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_HORSE_BACK] = ACTION(RideBack, "Horse_Move_Back", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_HORSE_JUMP] = ACTION(RideJump, "Horse_Jump_Gear_3", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_HORSE_REAR] = ACTION(RideRear, "Horse_Courbette_Gear_3", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_HORSE_RODEO] = ACTION(RideRodeo, "Horse_Rodeo", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_HORSE_SWORD_RIGHT] = ACTION(RideSwordRight, "Sword_Attack_Horse_R", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_HORSE_SWORD_LEFT] = ACTION(RideSwordLeft, "Sword_Attack_Horse_L", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_HORSE_LSWORD_RIGHT] = ACTION(RideLargeSwordRight, "Lsword_Attack_Horse_R", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_HORSE_LSWORD_LEFT] = ACTION(RideLargeSwordLeft, "Lsword_Attack_Horse_L", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_HORSE_SPEAR_RIGHT] = ACTION(RideSpearRight, "Spear_Attack_Horse_R30", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_HORSE_SPEAR_LEFT] = ACTION(RideSpearLeft, "Spear_Attack_Horse_L30_D40", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_HORSE_SWORD_CHARGE] = ACTION(RideSwordCharge, "Sword_Charge_Horse", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_HORSE_LSWORD_CHARGE] = ACTION(RideLargeSwordCharge, "Lsword_Charge_Horse", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_HORSE_SPEAR_CHARGE] = ACTION(RideSpearCharge, "Spear_Charge_Horse", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_HORSE_BOW_LOAD] = ACTION(RideBowLoad, "Bow_Attack_Load", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_HORSE_BOW_AIM] = ACTION(RideBowAim, "Bow_Attack_Charge", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_HORSE_BOW_SHOOT] = ACTION(RideBowShoot, "Bow_Attack_Shoot", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_HORSE_BOW_BACK_LOAD] = ACTION(RideBowBackLoad, "Bow_Attack_Load", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_HORSE_BOW_BACK_AIM] = ACTION(RideBowBackAim, "Bow_Attack_Charge", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_HORSE_BOW_BACK_SHOOT] = ACTION(RideBowBackShoot, "Bow_Attack_Shoot", NULL, PROXY_GEAR_NONE),
    [SWMP_PROXY_HORSE_BOW_END] = ACTION(RideBowEnd, "Bow_Attack_Horse_End", NULL, PROXY_GEAR_NONE),
};

static const ProxyAction horse_actions[] = {
    ACTION(HorseWait, "Wait", "Wait", PROXY_GEAR_NONE),
    ACTION(HorseWalk, "Move_Gear_1", "Move_Gear1", PROXY_GEAR_NONE),
    ACTION(HorseTrot, "Move_Gear_2", "Move_Gear2", PROXY_GEAR_NONE),
    ACTION(HorseCanter, "Move_Gear_3", "Move_Gear3", PROXY_GEAR_NONE),
    ACTION(HorseGallop, "Move_Gear_Top", "Move_Gear4", PROXY_GEAR_NONE),
    ACTION(HorseBack, "Move_Back", "Move_Back", PROXY_GEAR_NONE),
    ACTION(HorseJump, "Jump_Gear_3", "Jump_Gear3", PROXY_GEAR_NONE),
    ACTION(HorseRear, "Courbette_Gear_3", "Courbette", PROXY_GEAR_NONE),
    ACTION(HorseRodeo, "Rodeo", "Rodeo", PROXY_GEAR_NONE),
};

_Static_assert(sizeof horse_actions / sizeof *horse_actions == SWMP_HORSE_GAIT_COUNT - 1,
               "every horse gait needs a proxy clip");

const char *swmp_horse_gait_name(SwmpHorseGait gait) {
    return gait > SWMP_HORSE_NONE && gait < SWMP_HORSE_GAIT_COUNT ? horse_actions[gait - 1].state : NULL;
}

SwmpHorseGait swmp_horse_gait(const char *state) {
    for (unsigned i = 0; i < sizeof horse_actions / sizeof *horse_actions; i++)
        if (!strcmp(state, horse_actions[i].native)) return SWMP_HORSE_WAIT + i;
    if (!strncmp(state, "Jump", 4)) return SWMP_HORSE_JUMP;
    return SWMP_HORSE_NONE;
}

SwmpProxyAction swmp_horse_rider_action(SwmpHorseGait gait, const char *base, const char *lower, const char *upper,
                                       SwmpGearFamily family, bool left) {
    if (gait <= SWMP_HORSE_NONE || gait >= SWMP_HORSE_GAIT_COUNT) return SWMP_PROXY_IDLE;
    if (!strcmp(base, "CutHorseRide") || !strcmp(base, "CutChargeHorseRide") ||
        !strcmp(lower, "CutHorseRide") || !strcmp(lower, "CutChargeHorseRide") ||
        !strcmp(upper, "CutHorseRide") || !strcmp(upper, "CutChargeHorseRide")) {
        if (family == SWMP_GEAR_FAMILY_SWORD)
            return left ? SWMP_PROXY_HORSE_SWORD_LEFT : SWMP_PROXY_HORSE_SWORD_RIGHT;
        if (family == SWMP_GEAR_FAMILY_LSWORD)
            return left ? SWMP_PROXY_HORSE_LSWORD_LEFT : SWMP_PROXY_HORSE_LSWORD_RIGHT;
        if (family == SWMP_GEAR_FAMILY_SPEAR)
            return left ? SWMP_PROXY_HORSE_SPEAR_LEFT : SWMP_PROXY_HORSE_SPEAR_RIGHT;
    }
    if (!strcmp(base, "CutChargeWaitHorseRide") || !strcmp(lower, "CutChargeWaitHorseRide") ||
        !strcmp(upper, "CutChargeWaitHorseRide")) {
        if (family == SWMP_GEAR_FAMILY_SWORD) return SWMP_PROXY_HORSE_SWORD_CHARGE;
        if (family == SWMP_GEAR_FAMILY_LSWORD) return SWMP_PROXY_HORSE_LSWORD_CHARGE;
        if (family == SWMP_GEAR_FAMILY_SPEAR) return SWMP_PROXY_HORSE_SPEAR_CHARGE;
    }
    bool back = !strcmp(lower, "AimHorseLowerB") || !strcmp(lower, "AimHorseLowerWaitB") ||
        !strcmp(lower, "AimHorseLowerTurnB") || !strcmp(base, "AimHorseLowerB");
    if (!strcmp(upper, "BowReload")) return back ? SWMP_PROXY_HORSE_BOW_BACK_LOAD : SWMP_PROXY_HORSE_BOW_LOAD;
    if (!strcmp(upper, "BowCharge")) return back ? SWMP_PROXY_HORSE_BOW_BACK_AIM : SWMP_PROXY_HORSE_BOW_AIM;
    if (!strcmp(upper, "BowShoot")) return back ? SWMP_PROXY_HORSE_BOW_BACK_SHOOT : SWMP_PROXY_HORSE_BOW_SHOOT;
    if (!strcmp(upper, "HorseBowEndUpper")) return SWMP_PROXY_HORSE_BOW_END;
    return SWMP_PROXY_HORSE_WAIT + gait - SWMP_HORSE_WAIT;
}
#undef ACTION

const char *swmp_proxy_action_name(SwmpProxyAction action) {
    return proxy_actions[(unsigned)action < SWMP_PROXY_ACTION_COUNT ? action : SWMP_PROXY_IDLE].state;
}

SwmpProxyAction swmp_proxy_carry_action(SwmpProxyAction action, unsigned count) {
    if (!count) return action;
    bool many = count >= 3;
    if (action == SWMP_PROXY_WALK) return many ? SWMP_PROXY_WALK_HOLD_MANY : SWMP_PROXY_WALK_HOLD_FEW;
    if (action == SWMP_PROXY_RUN || action == SWMP_PROXY_DASH)
        return many ? SWMP_PROXY_RUN_HOLD_MANY : SWMP_PROXY_RUN_HOLD_FEW;
    return many ? SWMP_PROXY_HOLD_MANY : SWMP_PROXY_HOLD_FEW;
}

typedef struct {
    SwmpProxyAction draw, ready;
    int min_hand_y, max_hand_y;
} WeaponAnimation;

static const WeaponAnimation weapon_animations[SWMP_GEAR_FAMILY_COUNT] = {
    [SWMP_GEAR_FAMILY_LSWORD] = {SWMP_PROXY_AXE_DRAW, SWMP_PROXY_AXE_READY, 550, 1050},
    [SWMP_GEAR_FAMILY_SWORD] = {SWMP_PROXY_SWORD_DRAW, SWMP_PROXY_SWORD_READY, 550, 1050},
    // The spear hangs lower than the other weapons even when drawn.
    [SWMP_GEAR_FAMILY_SPEAR] = {SWMP_PROXY_SPEAR_DRAW, SWMP_PROXY_SPEAR_READY, 525, 950},
};

SwmpProxyAction swmp_proxy_throw_action(const char *state, SwmpGearFamily family, float clip_duration) {
    static const struct {
        SwmpProxyAction windup, aim, release, catch_action;
    } throws[SWMP_GEAR_FAMILY_COUNT] = {
        [SWMP_GEAR_FAMILY_SWORD] = {SWMP_PROXY_SWORD_THROW_WINDUP, SWMP_PROXY_SWORD_THROW_AIM,
                                   SWMP_PROXY_SWORD_THROW_RELEASE, SWMP_PROXY_SWORD_THROW_CATCH},
        [SWMP_GEAR_FAMILY_LSWORD] = {SWMP_PROXY_LSWORD_THROW_WINDUP, SWMP_PROXY_LSWORD_THROW_AIM,
                                    SWMP_PROXY_LSWORD_THROW_RELEASE, SWMP_PROXY_LSWORD_THROW_CATCH},
        [SWMP_GEAR_FAMILY_SPEAR] = {SWMP_PROXY_SPEAR_THROW_WINDUP, SWMP_PROXY_SPEAR_THROW_AIM,
                                   SWMP_PROXY_SPEAR_THROW_RELEASE, SWMP_PROXY_IDLE},
    };
    if ((unsigned)family >= SWMP_GEAR_FAMILY_COUNT) return SWMP_PROXY_IDLE;
    if (!strcmp(state, "WeaponThrowCharge")) {
        // The native controller keeps this state while the 36-frame aim loop plays.
        return clip_duration == 36.0f ? throws[family].aim : throws[family].windup;
    }
    if (!strcmp(state, "WeaponThrowCatch")) return throws[family].catch_action;
    if (!strcmp(state, "WeaponThrow")) {
        // Some implements swing on the throw input instead of leaving the hand.
        if (family == SWMP_GEAR_FAMILY_SWORD && clip_duration == 21.0f) return SWMP_PROXY_SWORD_SWING;
        if (family == SWMP_GEAR_FAMILY_LSWORD && clip_duration == 60.0f) return SWMP_PROXY_AXE_SWING;
        return throws[family].release;
    }
    return SWMP_PROXY_IDLE;
}

SwmpProxyAction swmp_proxy_sample_action(const char *state, const char *upper, SwmpGearSet gear,
                                         const SwmpGearPose *weapon_pose, const SwmpGearPose *bow_pose,
                                         bool shield_raised, float draw_duration) {
    if (!strcmp(state, "WallDashSt")) return SWMP_PROXY_CLIMB_ENTER;
    if (!strcmp(state, "WallOff")) return SWMP_PROXY_CLIMB_RELEASE;
    // Reactions still play if the shield has no visual alias or was destroyed by the hit.
    if (!strcmp(state, "GuardJust")) return SWMP_PROXY_SHIELD_PARRY;
    if (!strcmp(state, "GuardFailHit")) return SWMP_PROXY_SHIELD_HIT;
    if (!strcmp(state, "GuardBreak")) return SWMP_PROXY_SHIELD_BREAK;
    if (!strncmp(state, "Surfing", 7)) {
        if (!strcmp(state, "SurfingOnFall")) return SWMP_PROXY_SURF_MOUNT;
        for (unsigned i = SWMP_PROXY_SURF_MOUNT; i <= SWMP_PROXY_SURF_FAILED; i++)
            if (!strcmp(state, proxy_actions[i].native)) return (SwmpProxyAction)i;
    }
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
    SwmpGearFamily family = swmp_gear_family(gear.id[SWMP_GEAR_WEAPON]);
    const WeaponAnimation *weapon = &weapon_animations[family];
    if (!strcmp(state, "WeaponEquipOn")) {
        // BOTW 1.9.0 shares this state, but uses 16-, 20-, and 32-frame clips
        // for shield, bow, and melee draws. A stowed weapon can move during a bow draw.
        if (draw_duration == 16.0f && gear.id[SWMP_GEAR_SHIELD]) return SWMP_PROXY_SHIELD_DRAW;
        if (draw_duration == 20.0f && gear.id[SWMP_GEAR_BOW]) return SWMP_PROXY_BOW_DRAW;
        if (draw_duration == 32.0f) return weapon->draw;
        return SWMP_PROXY_IDLE;
    }
    // Native Wait covers both stowed and drawn idle.
    bool weapon_drawn = weapon_pose && weapon->ready &&
        weapon_pose->offset_mm[1] >= weapon->min_hand_y && weapon_pose->offset_mm[1] <= weapon->max_hand_y;
    bool attention = !strcmp(state, "WaitAttention");
    if (shield_raised && !strcmp(state, "MoveAttention")) return SWMP_PROXY_SHIELD_WALK_FORWARD;
    if (shield_raised && (attention || !strcmp(state, "TurnAttention") || !strcmp(state, "LookAroundAttention")))
        return SWMP_PROXY_SHIELD_GUARD;
    if (gear.id[SWMP_GEAR_BOW] && bow_pose) {
        const int16_t *bow = bow_pose->offset_mm;
        // The bow passes Link's left side on its way between the back and aiming pose.
        bool at_side = bow[0] >= 150 && bow[1] <= 1000;
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
        } else if (action->gear && (unsigned)action->gear != family) continue;
        if (!strcmp(name, action->native)) return (SwmpProxyAction)i;
    }
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
    PROXY_ANIM_STATE_SIZE = 448,
};

static size_t proxy_anim_list(uint8_t *out, const ProxyAction *actions, unsigned count, const char *archive) {
    enum { ROOT = 52, ADD_RES = ROOT + 12, DEFINES = ADD_RES + 12,
           RES_OBJECT = DEFINES + 12, FIRST_STATE = RES_OBJECT + 8 };
    unsigned params = FIRST_STATE + 8 * count;
    unsigned strings = params + 8 * (1 + 2 * count);
    aamp_list(out, ROOT, "param_root", ADD_RES, 2, 0, 0);
    aamp_list(out, ADD_RES, "AddReses", 0, 0, RES_OBJECT, 1);
    aamp_list(out, DEFINES, "ASDefines", 0, 0, FIRST_STATE, count);
    aamp_object(out, RES_OBJECT, "AddRes_0", params, 1);
    unsigned end = aamp_string(out, params, "Anim", strings, archive);
    const ProxyAction *order[SWMP_PROXY_ACTION_COUNT];
    for (unsigned i = 0; i < count; i++) order[i] = &actions[i];
    qsort(order, count, sizeof *order, compare_actions);
    for (unsigned i = 0; i < count; i++) {
        char entry[24];
        snprintf(entry, sizeof entry, "ASDefine_%u", i);
        unsigned param = params + 8 + 16 * i;
        aamp_object(out, FIRST_STATE + 8 * i, entry, param, 2);
        end = aamp_string(out, param, "Name", end, order[i]->state);
        end = aamp_string(out, param + 8, "Filename", end, order[i]->file);
    }
    aamp_header(out, end, 3, 1 + count, 1 + 2 * count, 0, end - strings);
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

static size_t proxy_masked_state(uint8_t out[PROXY_ANIM_STATE_SIZE], const char *gait,
                                 const char *upper, const char *mask) {
    enum { ROOT = 52, ELEMENTS = 64, BLEND = 76, GAIT = 88, HOLD = 100, EXTEND = 112,
           RANGES = 124, MASK = 136, OBJECTS = 148, PARAMS = 204, DATA = 316, STRINGS = 360 };
    aamp_list(out, ROOT, "param_root", ELEMENTS, 1, 0, 0);
    aamp_list(out, ELEMENTS, "Elements", BLEND, 3, 0, 0);
    aamp_list(out, BLEND, "Element0", EXTEND, 1, OBJECTS, 2);
    aamp_list(out, GAIT, "Element1", 0, 0, OBJECTS + 16, 1);
    aamp_list(out, HOLD, "Element2", 0, 0, OBJECTS + 24, 1);
    aamp_list(out, EXTEND, "Extend", RANGES, 2, 0, 0);
    aamp_list(out, RANGES, "Ranges", 0, 0, OBJECTS + 32, 2);
    aamp_list(out, MASK, "BlenderBone", 0, 0, OBJECTS + 48, 1);
    aamp_object(out, OBJECTS, "Parameters", PARAMS, 3);
    aamp_object(out, OBJECTS + 8, "Children", PARAMS + 24, 2);
    aamp_object(out, OBJECTS + 16, "Parameters", PARAMS + 40, 2);
    aamp_object(out, OBJECTS + 24, "Parameters", PARAMS + 56, 2);
    aamp_object(out, OBJECTS + 32, "Range0", PARAMS + 72, 2);
    aamp_object(out, OBJECTS + 40, "Range1", PARAMS + 88, 2);
    aamp_object(out, OBJECTS + 48, "BlenderBone0", PARAMS + 104, 1);
    aamp_value(out, PARAMS, "TypeIndex", 2, DATA);
    write32(out + DATA, 5);
    aamp_value(out, PARAMS + 8, "JudgeOnce", 0, DATA + 4);
    aamp_value(out, PARAMS + 16, "InputLimit", 1, DATA + 8);
    write32(out + DATA + 8, UINT32_C(0xbf800000));  // -1.0f
    aamp_value(out, PARAMS + 24, "Child0", 2, DATA + 12);
    aamp_value(out, PARAMS + 32, "Child1", 2, DATA + 16);
    write32(out + DATA + 12, 1);
    write32(out + DATA + 16, 2);
    aamp_value(out, PARAMS + 40, "TypeIndex", 2, DATA + 20);
    aamp_value(out, PARAMS + 56, "TypeIndex", 2, DATA + 24);
    write32(out + DATA + 20, 67);
    write32(out + DATA + 24, 67);
    unsigned end = aamp_string(out, PARAMS + 48, "FileName", STRINGS, gait);
    end = aamp_string(out, PARAMS + 64, "FileName", end, upper);
    aamp_value(out, PARAMS + 72, "Start", 1, DATA + 28);
    aamp_value(out, PARAMS + 80, "End", 1, DATA + 32);
    aamp_value(out, PARAMS + 88, "Start", 1, DATA + 36);
    aamp_value(out, PARAMS + 96, "End", 1, DATA + 40);
    unsigned mask_name = end;
    end = aamp_string(out, PARAMS + 104, "Value0", end, mask);
    aamp_value(out, PARAMS + 104, "Value0", 7, mask_name);
    aamp_header(out, end, 8, 7, 14, STRINGS - DATA, end - STRINGS);
    return end;
}

static uint8_t *proxy_actor_link(uint8_t *src, size_t size, const char *key, const char *value, size_t *out_size) {
    if (size < 52 || size > 4096 || (size & 3) || read32(src + 12) != size ||
        read32(src + 44) || !set_dummy_physics(src, size)) return NULL;
    uint8_t *param = aamp_param(src, size, key, 20);
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

static uint8_t *animation_offset(uint8_t *raw, size_t size, const uint8_t *field, size_t length) {
    uint64_t offset;
    memcpy(&offset, field, sizeof offset);
    return offset && offset <= size && length <= size - offset ? raw + offset : NULL;
}

static bool animation_name(uint8_t *raw, size_t size, const uint8_t *field, const char *name) {
    size_t length = strlen(name) + 1;
    const uint8_t *str = animation_offset(raw, size, field, 2 + length);
    return str && !memcmp(str + 2, name, length);
}

enum {
    ROOT_X = 1, ROOT_Y = 2, ROOT_Z = 4, ROOT_YAW = 8,
    ROOT_PLANAR = ROOT_X | ROOT_Z,
    ROOT_TRANSLATION = ROOT_PLANAR | ROOT_Y,
};

static bool remove_root_motion(uint8_t *raw, size_t size, const uint8_t *clip, unsigned motion) {
    // These clips use Root as the first bone with an explicit base transform.
    uint8_t *bone = animation_offset(raw, size, clip + 0x30, 0x28);
    if (!bone || !animation_name(raw, size, bone, "Root") || bone[0x1c] != 3 || bone[0x1d] != 6)
        return false;
    uint8_t *base = animation_offset(raw, size, bone + 0x10, 10 * sizeof(float));
    unsigned curve_count = bone[0x1e];
    uint8_t *curves = animation_offset(raw, size, bone + 8, curve_count * 0x30u);
    if (!base || !curves || !curve_count || curve_count > 10) return false;
    bool changed = false;
    for (unsigned i = 0; i < curve_count; i++) {
        uint8_t *curve = curves + i * 0x30;
        uint32_t target = read32(curve + 0x14);
        if ((target == 0x10 && (motion & ROOT_X)) ||
            (target == 0x14 && (motion & ROOT_Y)) ||
            (target == 0x18 && (motion & ROOT_Z)) ||
            (target == 0x24 && (motion & ROOT_YAW))) {
            // Snapshots already carry this movement. Zero the curve's scale, offset and loop delta.
            memset(curve + 0x20, 0, 3 * sizeof(float));
            changed = true;
        }
    }
    if (motion & ROOT_X) write32(base + 0x1c, 0);
    if (motion & ROOT_Y) write32(base + 0x20, 0);
    if (motion & ROOT_Z) write32(base + 0x24, 0);
    if (motion & ROOT_YAW) write32(base + 0x10, 0);
    return changed;
}

typedef struct {
    unsigned action, motion;
} RootMotion;

static uint8_t *in_place_animation(const uint8_t *src, size_t size, size_t *out_size,
                                   const ProxyAction *actions, const RootMotion *motion, unsigned motion_count) {
    if (!src || size < 16 || memcmp(src, "Yaz0", 4) || src[4] >= 4) return NULL;
    size_t n;
    uint8_t *raw = yaz0_unpack(src, size, &n);
    if (!raw) return NULL;
    if (n < 0xc0 || memcmp(raw, "FRES    \x03\0\x05\0\xff\xfe", 14) || read32(raw + 0x1c) != n)
        goto invalid;
    // Switch BFRES 5.0.3 stores contiguous skeletal-clip headers with file-relative offsets.
    unsigned count = raw[0xbe] | raw[0xbf] << 8;
    uint8_t *clips = animation_offset(raw, n, raw + 0x38, count * 0x60u);
    if (!clips) goto invalid;
    unsigned found = 0;
    for (unsigned i = 0; i < count; i++) {
        const uint8_t *clip = clips + i * 0x60;
        if (memcmp(clip, "FSKA", 4)) goto invalid;
        for (unsigned j = 0; j < motion_count; j++) {
            if (!animation_name(raw, n, clip + 0x10, actions[motion[j].action].clip)) continue;
            if ((found & (1u << j)) || !remove_root_motion(raw, n, clip, motion[j].motion)) goto invalid;
            found |= 1u << j;
        }
    }
    if (found != (1u << motion_count) - 1) goto invalid;
    *out_size = n;
    return raw;
invalid:
    free(raw);
    return NULL;
}

uint8_t *swmp_proxy_animation(const uint8_t *src, size_t size, size_t *out_size) {
    static const RootMotion root_motion[] = {
        {SWMP_PROXY_WALK, ROOT_PLANAR},
        {SWMP_PROXY_RUN, ROOT_PLANAR},
        {SWMP_PROXY_DASH, ROOT_PLANAR},
        {SWMP_PROXY_CROUCH_MOVE, ROOT_PLANAR},
        {SWMP_PROXY_SHIELD_WALK_FORWARD, ROOT_PLANAR},
        {SWMP_PROXY_SHIELD_WALK_BACK, ROOT_PLANAR},
        {SWMP_PROXY_SHIELD_WALK_LEFT, ROOT_PLANAR},
        {SWMP_PROXY_SHIELD_WALK_RIGHT, ROOT_PLANAR},
        {SWMP_PROXY_SWIM_DASH, ROOT_PLANAR},
        {SWMP_PROXY_SWIM_SLOW_LEFT, ROOT_PLANAR | ROOT_YAW},
        {SWMP_PROXY_SWIM_SLOW_RIGHT, ROOT_PLANAR | ROOT_YAW},
        {SWMP_PROXY_SWIM_FAST_LEFT, ROOT_PLANAR | ROOT_YAW},
        {SWMP_PROXY_SWIM_FAST_RIGHT, ROOT_PLANAR | ROOT_YAW},
        {SWMP_PROXY_CLIMB_MOVE, ROOT_TRANSLATION},
        {SWMP_PROXY_CLIMB_RIGHT, ROOT_TRANSLATION},
        {SWMP_PROXY_CLIMB_UP_RIGHT, ROOT_TRANSLATION},
        {SWMP_PROXY_CLIMB_UP_LEFT, ROOT_TRANSLATION},
        {SWMP_PROXY_CLIMB_LEFT, ROOT_TRANSLATION},
        {SWMP_PROXY_CLIMB_DOWN_LEFT, ROOT_TRANSLATION},
        {SWMP_PROXY_CLIMB_DOWN, ROOT_TRANSLATION},
        {SWMP_PROXY_CLIMB_DOWN_RIGHT, ROOT_TRANSLATION},
        {SWMP_PROXY_CLIMB_JUMP, ROOT_TRANSLATION},
        {SWMP_PROXY_CLIMB_JUMP_RIGHT, ROOT_TRANSLATION},
        {SWMP_PROXY_CLIMB_JUMP_UP_RIGHT, ROOT_TRANSLATION},
        {SWMP_PROXY_CLIMB_JUMP_UP_LEFT, ROOT_TRANSLATION},
        {SWMP_PROXY_CLIMB_JUMP_LEFT, ROOT_TRANSLATION},
    };
    return in_place_animation(src, size, out_size, proxy_actions, root_motion, sizeof root_motion / sizeof *root_motion);
}

uint8_t *swmp_horse_animation(const uint8_t *src, size_t size, size_t *out_size) {
    static const RootMotion root_motion[] = {
        {SWMP_HORSE_WALK - 1, ROOT_PLANAR},
        {SWMP_HORSE_TROT - 1, ROOT_PLANAR},
        {SWMP_HORSE_CANTER - 1, ROOT_PLANAR},
        {SWMP_HORSE_GALLOP - 1, ROOT_PLANAR},
        {SWMP_HORSE_BACK - 1, ROOT_PLANAR},
        {SWMP_HORSE_JUMP - 1, ROOT_PLANAR},
        {SWMP_HORSE_REAR - 1, ROOT_PLANAR},
    };
    return in_place_animation(src, size, out_size, horse_actions, root_motion, sizeof root_motion / sizeof *root_motion);
}

static const char stock_link[] = "Actor/ActorLink/Obj_RockGray_A_M_01.bxml";
static const char stock_model_list[] = "Actor/ModelList/Obj_RockGray_A_M_01.bmodellist";

static uint8_t *proxy_model_list(uint8_t *stock, size_t size, const char *folder, const char *unit,
                                 const char *palette, size_t *out_size) {
    if (size < 52 || size > 4096 || (size & 3) || read32(stock + 12) != size || read32(stock + 44)) return NULL;
    uint8_t *folder_param = aamp_param(stock, size, "Folder", 8);
    uint8_t *unit_param = aamp_param(stock, size, "UnitName", 8);
    uint8_t *material = aamp_param(stock, size, "VariationMatAnim", 8);
    uint8_t *shader = aamp_param(stock, size, "VariationShaderAnim", 8);
    if (!folder_param || !unit_param || (palette && (!material || !shader))) return NULL;
    size_t capacity = size + align4(strlen(folder) + 1) + align4(strlen(unit) + 1);
    if (palette) capacity += 2 * align4(strlen(palette) + 1);
    uint8_t *model = calloc(1, capacity);
    if (!model) return NULL;
    memcpy(model, stock, size);
    unsigned end = aamp_string(model, (unsigned)(folder_param - stock), "Folder", (unsigned)size, folder);
    end = aamp_string(model, (unsigned)(unit_param - stock), "UnitName", end, unit);
    if (palette) {
        end = aamp_string(model, (unsigned)(material - stock), "VariationMatAnim", end, palette);
        end = aamp_string(model, (unsigned)(shader - stock), "VariationShaderAnim", end, palette);
    }
    write32(model + 12, end);
    write32(model + 40, read32(stock + 40) + (uint32_t)(end - size));
    *out_size = end;
    return model;
}

static uint8_t *animated_proxy_pack(const char *actor, PackFile link, PackFile model,
                                    bool in_place, size_t *out_size) {
    uint8_t list[PROXY_ANIM_LIST_SIZE] = {0}, clips[SWMP_PROXY_ACTION_COUNT][PROXY_ANIM_STATE_SIZE] = {{0}};
    char path[80];
    snprintf(path, sizeof path, "Actor/ASList/%s.baslist", actor);
    PackFile files[3 + SWMP_PROXY_ACTION_COUNT] = {
        link, model, {path, list, proxy_anim_list(list, proxy_actions, SWMP_PROXY_ACTION_COUNT,
                                                in_place ? SWMP_PROXY_ANIMATION : SWMP_PLAYER_ANIMATION)},
    };
    for (unsigned i = 0; i < SWMP_PROXY_ACTION_COUNT; i++) {
        const char *gait = NULL;
        if (i == SWMP_PROXY_WALK_HOLD_FEW || i == SWMP_PROXY_WALK_HOLD_MANY)
            gait = proxy_actions[SWMP_PROXY_WALK].clip;
        if (i == SWMP_PROXY_RUN_HOLD_FEW || i == SWMP_PROXY_RUN_HOLD_MANY)
            gait = proxy_actions[SWMP_PROXY_RUN].clip;
        const char *mask = "Lift";
        if (i >= SWMP_PROXY_HORSE_BOW_LOAD && i <= SWMP_PROXY_HORSE_BOW_BACK_SHOOT) {
            gait = i >= SWMP_PROXY_HORSE_BOW_BACK_LOAD ?
                "Bow_Attack_Horse_Draw_Lower_B" : "Bow_Attack_Horse_Draw_Lower_F";
            mask = "ArmLR";
        }
        size_t bytes = gait ? proxy_masked_state(clips[i], gait, proxy_actions[i].clip, mask) :
            proxy_anim_state(clips[i], proxy_actions[i].clip);
        files[3 + i] = (PackFile){proxy_actions[i].path, clips[i], bytes};
    }
    return pack_files(files, sizeof files / sizeof files[0], out_size);
}

uint8_t *swmp_proxy_pack(const uint8_t *src, size_t size, bool in_place, size_t *out_size) {
    static const char proxy_link[] = "Actor/ActorLink/" SWMP_PROXY_ACTOR ".bxml";
    size_t n;
    uint8_t *raw = src ? yaz0_unpack(src, size, &n) : NULL;
    if (!raw) return NULL;
    uint32_t data, ls, le, ms, me;
    uint8_t *out = NULL;
    if (sarc_find(raw, n, stock_link, &data, &ls, &le) >= 0 &&
        sarc_find(raw, n, stock_model_list, &data, &ms, &me) >= 0) {
        size_t link_size, model_size;
        uint8_t *link = proxy_actor_link(raw + data + ls, le - ls, "ASUser", SWMP_PROXY_ACTOR, &link_size);
        if (link) {
            size_t effect_size;
            uint8_t *effect_link = proxy_actor_link(link, link_size, "ElinkUser", link_elink, &effect_size);
            if (effect_link) {
                free(link);
                link = effect_link;
                link_size = effect_size;
            }
        }
        uint8_t *model = proxy_model_list(raw + data + ms, me - ms, link_model, link_model, NULL, &model_size);
        if (link && model) {
            out = animated_proxy_pack(SWMP_PROXY_ACTOR, (PackFile){proxy_link, link, link_size},
                (PackFile){stock_model_list, model, model_size}, in_place, out_size);
        }
        free(model);
        free(link);
    }
    free(raw);
    return out;
}

static const char *const armor_palette[SWMP_ARMOR_SLOTS] = {
    [SWMP_ARMOR_HEAD] = "Head", [SWMP_ARMOR_UPPER] = "Upper", [SWMP_ARMOR_LOWER] = "Lower",
};

uint8_t *swmp_armor_pack(const SwmpArmorAsset *asset, const uint8_t *src, size_t size,
                         bool in_place, size_t *out_size) {
    size_t n;
    uint8_t *raw = src ? yaz0_unpack(src, size, &n) : NULL;
    if (!raw) return NULL;
    uint32_t data, ls, le, ms, me;
    uint8_t *out = NULL, *model = NULL, *linked = NULL, *animated = NULL;
    if (sarc_find(raw, n, stock_link, &data, &ls, &le) < 0 ||
        sarc_find(raw, n, stock_model_list, &data, &ms, &me) < 0) goto done;

    size_t link_size, animated_size;
    linked = proxy_actor_link(raw + data + ls, le - ls, "ModelUser", asset->actor, &link_size);
    if (!linked) goto done;
    animated = proxy_actor_link(linked, link_size, "ASUser", asset->actor, &animated_size);
    if (!animated) goto done;

    // These variation names reserve the native material tracks alongside the skeletal AS.
    const char *palette = armor_palette[asset->slot];
    size_t model_size;
    model = proxy_model_list(raw + data + ms, me - ms, asset->folder, asset->model, palette, &model_size);
    if (!model) goto done;

    char link_path[80], model_path[80];
    snprintf(link_path, sizeof link_path, "Actor/ActorLink/%s.bxml", asset->actor);
    snprintf(model_path, sizeof model_path, "Actor/ModelList/%s.bmodellist", asset->actor);
    out = animated_proxy_pack(asset->actor, (PackFile){link_path, animated, animated_size},
        (PackFile){model_path, model, model_size}, in_place, out_size);
done:
    free(animated);
    free(linked);
    free(model);
    free(raw);
    return out;
}

static const char *gear_user(uint8_t *link, size_t size, const char *key) {
    const uint8_t *param = aamp_param(link, size, key, 20);
    if (!param) return NULL;
    size_t offset = (size_t)(param - link) + (read32(param + 4) & 0xffffff) * 4;
    if (offset >= size) return NULL;
    const char *value = (const char *)link + offset;
    return memchr(value, 0, size - offset) && resource_name(value, 48) ? value : NULL;
}

// Keep the native AS list and its clip names together. Some shields and bows use
// a custom list whose clips mix the item's name and shared resource names.
static unsigned gear_animations(PackFile *files, unsigned capacity, const char *alias, const char *user,
                                const uint8_t *archive, size_t size) {
    char source[80];
    snprintf(source, sizeof source, "Actor/ASList/%s.baslist", user);
    uint32_t data, start, end;
    if (sarc_find(archive, size, source, &data, &start, &end) < 0) return 0;
    files[0] = (PackFile){alias, archive + data + start, end - start};
    unsigned count = 1;
    int nodes = sarc_count(archive, size);
    for (int i = 0; i < nodes; i++) {
        const char *name;
        const uint8_t *clip;
        size_t length;
        if (!sarc_node(archive, size, i, &name, &clip, &length) || !name || strncmp(name, "Actor/AS/", 9))
            continue;
        size_t name_length = strlen(name);
        if (name_length < 14 || strcmp(name + name_length - 4, ".bas")) continue;
        if (count == capacity) return 0;
        files[count++] = (PackFile){name, clip, length};
    }
    // Ordinary shields share an empty EquipItemCommon list.
    return count > 1 ? count : 0;
}

static uint8_t *horse_pack(const SwmpGearAsset *asset, uint8_t *link, size_t link_size, PackFile model,
                           size_t *out_size) {
    enum { COUNT = SWMP_HORSE_GAIT_COUNT - 1 };
    uint8_t *animated = proxy_actor_link(link, link_size, "ASUser", asset->actor, &link_size);
    if (!animated) return NULL;
    char path[80];
    snprintf(path, sizeof path, "Actor/ASList/%s.baslist", asset->actor);
    uint8_t list[124 + COUNT * (24 + 2 * 64)] = {0}, clips[COUNT][PROXY_ANIM_STATE_SIZE] = {{0}};
    PackFile files[3 + COUNT] = {
        {asset->actor_link, animated, link_size}, model,
        {path, list, proxy_anim_list(list, horse_actions, COUNT, SWMP_PROXY_HORSE_ANIMATION)},
    };
    for (unsigned i = 0; i < COUNT; i++)
        files[3 + i] = (PackFile){horse_actions[i].path, clips[i], proxy_anim_state(clips[i], horse_actions[i].clip)};
    uint8_t *out = pack_files(files, sizeof files / sizeof files[0], out_size);
    free(animated);
    return out;
}

static uint8_t *gear_pack(const SwmpGearAsset *asset, const char *native,
                          const uint8_t *src, size_t size, const uint8_t *gear, size_t gear_size,
                          size_t *out_size, bool *animated) {
    if (animated) *animated = false;
    size_t n, gn;
    uint8_t *raw = src ? yaz0_unpack(src, size, &n) : NULL;
    uint8_t *model = gear ? yaz0_unpack(gear, gear_size, &gn) : NULL;
    uint8_t *out = NULL;
    uint32_t data, ls, le, model_data, ms, me;
    const char *model_user = asset->model_user, *model_list = asset->model_list;
    const char *anim_user = asset->slot == SWMP_PARAGLIDER_SLOT ? "PlayerStole" : NULL;
    char native_list[80];
    if (native && model) {
        char native_link[80];
        snprintf(native_link, sizeof native_link, "Actor/ActorLink/%s.bxml", native);
        uint32_t start, end;
        if (sarc_find(model, gn, native_link, &model_data, &start, &end) < 0) goto done;
        uint8_t *link = model + model_data + start;
        model_user = gear_user(link, end - start, "ModelUser");
        if (!model_user) goto done;
        snprintf(native_list, sizeof native_list, "Actor/ModelList/%s.bmodellist", model_user);
        model_list = native_list;
        if (asset->slot == SWMP_GEAR_BOW || asset->slot == SWMP_GEAR_SHIELD)
            anim_user = gear_user(link, end - start, "ASUser");
    }
    // Keep the prop profile and dummy physics so the visual stays out of pickup, inventory and hit processing.
    if (raw && model && model_list && sarc_find(raw, n, stock_link, &data, &ls, &le) >= 0 &&
        sarc_find(model, gn, model_list, &model_data, &ms, &me) >= 0) {
        size_t link_size;
        uint8_t *link = proxy_actor_link(raw + data + ls, le - ls, "ModelUser", model_user, &link_size);
        if (link) {
            enum { BASE_FILES = 2, MAX_ANIMATION_FILES = 32 };
            PackFile files[BASE_FILES + MAX_ANIMATION_FILES] = {
                {asset->actor_link, link, link_size},
                {model_list, model + model_data + ms, me - ms},
            };
            if (asset->slot == SWMP_HORSE_SLOT) {
                out = horse_pack(asset, link, link_size, files[1], out_size);
                free(link);
                goto done;
            }
            unsigned count = BASE_FILES;
            char alias_list[80];
            if (anim_user) {
                snprintf(alias_list, sizeof alias_list, "Actor/ASList/%s.baslist", asset->actor);
                unsigned animation_count = gear_animations(files + BASE_FILES, MAX_ANIMATION_FILES,
                                                           alias_list, anim_user, model, gn);
                uint8_t *linked = animation_count ?
                    proxy_actor_link(link, link_size, "ASUser", asset->actor, &link_size) : NULL;
                if (linked) {
                    free(link);
                    link = linked;
                    files[0] = (PackFile){asset->actor_link, link, link_size};
                    count += animation_count;
                }
            }
            if (asset->slot != SWMP_PARAGLIDER_SLOT || count > BASE_FILES)
                out = pack_files(files, count, out_size);
            if (out && animated) *animated = count > BASE_FILES;
            free(link);
        }
    }
done:
    free(model);
    free(raw);
    return out;
}

uint8_t *swmp_gear_pack(const SwmpGearAsset *asset, const uint8_t *src, size_t size,
                        const uint8_t *gear, size_t gear_size, size_t *out_size) {
    return gear_pack(asset, NULL, src, size, gear, gear_size, out_size, NULL);
}

uint8_t *swmp_gear_model_pack(SwmpGearModel *asset, const uint8_t *src, size_t size,
                              const uint8_t *gear, size_t gear_size, size_t *out_size) {
    char actor_link[80];
    snprintf(actor_link, sizeof actor_link, "Actor/ActorLink/%s.bxml", asset->actor);
    SwmpGearFamily family = asset->id <= UINT16_MAX ? swmp_gear_family((uint16_t)asset->id) : SWMP_GEAR_FAMILY_NONE;
    SwmpGearAsset visual = {.slot = gear_families[family].slot,
                            .actor = asset->actor, .actor_link = actor_link};
    return gear_pack(&visual, asset->name, src, size, gear, gear_size, out_size, &asset->animated);
}
#endif
