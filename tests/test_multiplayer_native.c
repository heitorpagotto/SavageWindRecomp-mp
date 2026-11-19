#include "native_abi_draft.h"

#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#endif

static void SWMP_NATIVE_CALL check_log(void *context, const char *message) {
    static const char *const expected[] = {"init", "poll", "reset", "poll", "reset", "shutdown"};
    unsigned *step = context;
    assert(*step < sizeof expected / sizeof *expected && strcmp(message, expected[*step]) == 0);
    (*step)++;
}

// Only the project-built test fixture path is supplied by the test recipe.
int main(int argc, char **argv) {
    if (argc == 1) {
        puts("multiplayer native fixture: disabled");
        return 0;
    }
    if (argc != 2) return 2;
#ifdef _WIN32
    HMODULE library = LoadLibraryA(argv[1]);
    assert(library);
    SwmpNativeQueryDraft query = (SwmpNativeQueryDraft)(void (*)(void))GetProcAddress(library, SWMP_NATIVE_ENTRY);
#else
    void *library = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    assert(library);
    SwmpNativeQueryDraft query = (SwmpNativeQueryDraft)dlsym(library, SWMP_NATIVE_ENTRY);
#endif
    assert(query && !query(SWMP_NATIVE_ABI_DRAFT + 1));
    const SwmpNativeModuleDraft *module = query(SWMP_NATIVE_ABI_DRAFT);
    assert(module && module->abi_version == SWMP_NATIVE_ABI_DRAFT && module->size >= sizeof *module);
    assert(module->init && module->poll && module->reset && module->shutdown);
    unsigned step = 0;
    SwmpNativeHostDraft host = {
        .abi_version = SWMP_NATIVE_ABI_DRAFT,
        .size = sizeof host,
        .context = &step,
        .log = check_log,
    };
    assert(!module->init(&host));
    host.capabilities = SWMP_NATIVE_CAP_LIFECYCLE;
    assert((module->required_host_capabilities & ~host.capabilities) == 0);
    host.size--;
    assert(!module->init(&host));
    host.size++;
    void *instance = module->init(&host);
    assert(instance);
    module->poll(instance, 100);
    module->reset(instance);
    module->poll(instance, 1);
    module->shutdown(instance);
    assert(step == 6);
#ifdef _WIN32
    assert(FreeLibrary(library));
#else
    assert(dlclose(library) == 0);
#endif
    puts("multiplayer native fixture: lifecycle ok");
    return 0;
}
