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
    uint8_t *data;
    size_t size;
} PackFile;

static uint32_t sarc_hash(const char *name) {
    uint32_t hash = 0;
    for (; *name; name++) hash = hash * 0x65 + (unsigned char)*name;
    return hash;
}

static uint8_t *pack_files(PackFile a, PackFile b, size_t *size) {
    if (sarc_hash(a.name) > sarc_hash(b.name)) {
        PackFile swap = a;
        a = b;
        b = swap;
    }
    size_t names = 0x48, names_size = align4(strlen(a.name) + 1) + align4(strlen(b.name) + 1);
    size_t data = (names + names_size + 63) & ~(size_t)63;
    *size = data + align4(a.size) + b.size;
    uint8_t *out = calloc(1, *size);
    if (!out) return NULL;

    memcpy(out, "SARC\x14\0\xff\xfe", 8);
    write32(out + 8, (uint32_t)*size);
    write32(out + 12, (uint32_t)data);
    write32(out + 16, 0x100);
    memcpy(out + 20, "SFAT\x0c\0\x02\0", 8);
    write32(out + 28, 0x65);
    memcpy(out + 0x40, "SFNT\x08\0\0\0", 8);
    PackFile files[] = {a, b};
    for (unsigned i = 0, name_offset = 0, data_offset = 0; i < 2; i++) {
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
        set_dummy_physics(raw + data + ls, le - ls) &&
        replace_model_name(raw + data + ms, me - ms, model_archive) &&
        replace_model_name(raw + data + ms, me - ms, template_actor)) {
        PackFile link = {proxy_link, raw + data + ls, le - ls};
        PackFile model = {model_list, raw + data + ms, me - ms};
        out = pack_files(link, model, out_size);
    }
    free(raw);
    return out;
}
#endif
