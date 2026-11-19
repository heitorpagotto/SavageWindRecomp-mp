#include "native_abi_draft.h"

#include <stdlib.h>

#if defined(_WIN32)
#define SWMP_FIXTURE_EXPORT __declspec(dllexport)
#else
#define SWMP_FIXTURE_EXPORT __attribute__((visibility("default")))
#endif

typedef struct {
    const SwmpNativeHostDraft *host;
    uint64_t last_poll;
} Fixture;

static void *SWMP_NATIVE_CALL init(const SwmpNativeHostDraft *host) {
    if (!host || host->abi_version != SWMP_NATIVE_ABI_DRAFT || host->size < sizeof *host ||
        !(host->capabilities & SWMP_NATIVE_CAP_LIFECYCLE) || !host->log)
        return NULL;
    Fixture *fixture = malloc(sizeof *fixture);
    if (!fixture) return NULL;
    *fixture = (Fixture){.host = host};
    host->log(host->context, "init");
    return fixture;
}

static void SWMP_NATIVE_CALL poll(void *instance, uint64_t now_ms) {
    Fixture *fixture = instance;
    fixture->host->log(fixture->host->context, now_ms < fixture->last_poll ? "clock error" : "poll");
    fixture->last_poll = now_ms;
}

static void SWMP_NATIVE_CALL reset(void *instance) {
    Fixture *fixture = instance;
    fixture->last_poll = 0;
    fixture->host->log(fixture->host->context, "reset");
}

static void SWMP_NATIVE_CALL shutdown(void *instance) {
    Fixture *fixture = instance;
    reset(fixture);
    fixture->host->log(fixture->host->context, "shutdown");
    free(fixture);
}

SWMP_FIXTURE_EXPORT const SwmpNativeModuleDraft *SWMP_NATIVE_CALL swmp_query_draft(uint32_t version) {
    static const SwmpNativeModuleDraft module = {
        .abi_version = SWMP_NATIVE_ABI_DRAFT,
        .size = sizeof module,
        .required_host_capabilities = SWMP_NATIVE_CAP_LIFECYCLE,
        .init = init,
        .poll = poll,
        .reset = reset,
        .shutdown = shutdown,
    };
    return version == SWMP_NATIVE_ABI_DRAFT ? &module : NULL;
}
