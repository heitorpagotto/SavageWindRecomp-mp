// Proposed standalone C ABI. See .milestones/NATIVE_ABI.md for scope and ownership.
#pragma once

#include <stdint.h>

#define SWMP_NATIVE_ABI_DRAFT 1u
#define SWMP_NATIVE_ENTRY "swmp_query_draft"
#define SWMP_NATIVE_CAP_LIFECYCLE UINT64_C(1)

#if defined(_WIN32)
#define SWMP_NATIVE_CALL __cdecl
#else
#define SWMP_NATIVE_CALL
#endif

typedef struct {
    uint32_t abi_version;
    uint32_t size;
    uint64_t capabilities;
    void *context;
    void (SWMP_NATIVE_CALL *log)(void *context, const char *message);
} SwmpNativeHostDraft;

typedef struct {
    uint32_t abi_version;
    uint32_t size;
    uint64_t required_host_capabilities;
    void *(SWMP_NATIVE_CALL *init)(const SwmpNativeHostDraft *host);
    void (SWMP_NATIVE_CALL *poll)(void *instance, uint64_t now_ms);
    void (SWMP_NATIVE_CALL *reset)(void *instance);
    void (SWMP_NATIVE_CALL *shutdown)(void *instance);
} SwmpNativeModuleDraft;

typedef const SwmpNativeModuleDraft *(SWMP_NATIVE_CALL *SwmpNativeQueryDraft)(uint32_t abi_version);
