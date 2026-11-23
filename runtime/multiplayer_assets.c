#include "multiplayer_assets.h"

#ifdef SW_MULTIPLAYER
#include "services/archive.h"

#include <stdlib.h>
#include <string.h>
#include <zlib.h>

static const char template_actor[] = "Obj_RockGray_A_M_01";
static const char model_archive[] = "Obj_RockGray_A";
static const char link_model[] = "Link";

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

static uint8_t *insert_string(Byml *b, BymlNode strings, uint32_t index) {
    uint32_t start = read32(strings.values + 4 * index);
    uint32_t end = read32(strings.values + 4 * strings.count);
    size_t extra = sizeof SWMP_PROXY_ACTOR;
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
    memcpy(out + start + 4, SWMP_PROXY_ACTOR, extra);
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

static bool add_actor(Byml *b) {
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

    uint32_t name_index = 0, link_index = strings.count;
    for (uint32_t i = 0; i < strings.count; i++) {
        const char *s = string_at(b, strings, i);
        if (!s) return false;
        int order = strcmp(s, SWMP_PROXY_ACTOR);
        if (!order) return false;
        if (order < 0) name_index++;
        if (!strcmp(s, link_model)) link_index = i;
    }
    if (link_index == strings.count) return false;
    link_index += link_index >= name_index;

    uint32_t insert = 0, alias_crc = crc(SWMP_PROXY_ACTOR), stock_crc = crc(template_actor);
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

    uint8_t *new_strings = insert_string(b, strings, name_index);
    size_t actor_size = 4 + 8 * stock.count, root_size = 4 + 8 * root.count;
    uint8_t *actor = append(b, actor_size);
    if (!actor || !new_strings) return false;
    memcpy(actor, stock.head, actor_size);
    write32(actor + (name - stock.head), name_index);
    write32(actor + (bfres - stock.head), link_index);
    write32(actor + (model - stock.head), link_index);
    // The static prop template has a much smaller instance heap than the skinned model.
    write32(actor + (heap - stock.head), 2 * 1024 * 1024);

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

uint8_t *swmp_proxy_actor_info(const uint8_t *src, size_t size, size_t *out_size) {
    size_t n;
    uint8_t *raw = src ? yaz0_unpack(src, size, &n) : NULL;
    if (!raw || n < 16 || n > 16 * 1024 * 1024) {
        free(raw);
        return NULL;
    }
    Byml b = {.original_size = n, .length = n, .capacity = 4 * n + 512};
    b.data = calloc(1, b.capacity);
    if (b.data) memcpy(b.data, raw, n);
    free(raw);
    if (!b.data || !add_actor(&b)) {
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

// Keep state names sorted for native lookup. Only references to installed clips go into the alias.
static const struct {
    const char *state, *file, *path, *clip;
} proxy_actions[SWMP_PROXY_ACTION_COUNT] = {
    [SWMP_PROXY_IDLE] = {"Idle", "SwmpProxyIdle", "Actor/AS/SwmpProxyIdle.bas", "Nml_Wait"},
    [SWMP_PROXY_JUMP] = {"Jump", "SwmpProxyJump", "Actor/AS/SwmpProxyJump.bas", "Nml_Wait_Jump"},
    [SWMP_PROXY_WALK] = {"Walk", "SwmpProxyWalk", "Actor/AS/SwmpProxyWalk.bas", "Nml_Move_Walk"},
};

const char *swmp_proxy_action_name(SwmpProxyAction action) {
    return proxy_actions[(unsigned)action < SWMP_PROXY_ACTION_COUNT ? action : SWMP_PROXY_IDLE].state;
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

static size_t proxy_anim_list(uint8_t out[288]) {
    enum { ROOT = 52, ADD_RES = ROOT + 12, DEFINES = ADD_RES + 12,
           RES_OBJECT = DEFINES + 12, FIRST_STATE = RES_OBJECT + 8,
           PARAMS = FIRST_STATE + 8 * SWMP_PROXY_ACTION_COUNT,
           STRINGS_AT = PARAMS + 8 * (1 + 2 * SWMP_PROXY_ACTION_COUNT) };
    aamp_list(out, ROOT, "param_root", ADD_RES, 2, 0, 0);
    aamp_list(out, ADD_RES, "AddReses", 0, 0, RES_OBJECT, 1);
    aamp_list(out, DEFINES, "ASDefines", 0, 0, FIRST_STATE, SWMP_PROXY_ACTION_COUNT);
    aamp_object(out, RES_OBJECT, "AddRes_0", PARAMS, 1);
    unsigned end = aamp_string(out, PARAMS, "Anim", STRINGS_AT, "Player_Animation");
    static const char *const entries[] = {"ASDefine_0", "ASDefine_1", "ASDefine_2"};
    for (unsigned i = 0; i < SWMP_PROXY_ACTION_COUNT; i++) {
        unsigned param = PARAMS + 8 + 16 * i;
        aamp_object(out, FIRST_STATE + 8 * i, entries[i], param, 2);
        end = aamp_string(out, param, "Name", end, proxy_actions[i].state);
        end = aamp_string(out, param + 8, "Filename", end, proxy_actions[i].file);
    }
    aamp_header(out, end, 3, 1 + SWMP_PROXY_ACTION_COUNT, 1 + 2 * SWMP_PROXY_ACTION_COUNT,
                0, end - STRINGS_AT);
    return end;
}

static size_t proxy_anim_state(uint8_t out[144], const char *clip) {
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

static uint8_t *proxy_actor_link(uint8_t *src, size_t size, size_t *out_size) {
    if (size < 52 || size > 4096 || (size & 3) || read32(src + 12) != size ||
        read32(src + 44) || !set_dummy_physics(src, size)) return NULL;
    uint8_t *as = aamp_param(src, size, "ASUser");
    if (!as) return NULL;
    *out_size = size + align4(sizeof SWMP_PROXY_ACTOR);
    uint8_t *out = calloc(1, *out_size);
    if (!out) return NULL;
    memcpy(out, src, size);
    memcpy(out + size, SWMP_PROXY_ACTOR, sizeof SWMP_PROXY_ACTOR);
    aamp_value(out, (unsigned)(as - src), "ASUser", 20, (unsigned)size);
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
        uint8_t *link = proxy_actor_link(raw + data + ls, le - ls, &link_size);
        if (link) {
            uint8_t list[288] = {0}, clips[SWMP_PROXY_ACTION_COUNT][144] = {{0}};
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
#endif
