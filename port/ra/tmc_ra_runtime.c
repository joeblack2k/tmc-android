#include "tmc_ra_runtime.h"

#include "port_version.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

#define TMC_RA_USER_AGENT "tmc/" TMC_PC_VERSION " native-ra/1 rcheevos/12.3.0"

TmcRaRuntime gTmcRaRuntime;
static bool sAtexitRegistered;

static bool IsOwner(const TmcRaRuntime* runtime) {
    return runtime != NULL && runtime->owner_thread_set && pthread_equal(runtime->owner_thread, pthread_self());
}

static uint64_t Now(void* userdata) {
    (void)userdata;
    return (uint64_t)clock() * 1000u / CLOCKS_PER_SEC;
}

static void ShutdownAtExit(void) {
    TmcRaRuntime_Shutdown(&gTmcRaRuntime);
}

bool TmcRaRuntime_Init(TmcRaRuntime* runtime, const NRA_PlatformVTable* platform, void* platform_userdata) {
    const NRA_CreateParams params = {
        .abi_version = NRA_ABI_VERSION,
        .client_name = "The Minish Cap PC Port",
        .client_version = TMC_PC_VERSION,
        .user_agent = TMC_RA_USER_AGENT,
        .platform = platform,
        .platform_userdata = platform_userdata,
        .game = TmcRaAdapter_VTable(),
        .game_userdata = runtime ? &runtime->adapter : NULL,
    };

    if (runtime == NULL || platform == NULL || runtime->context != NULL)
        return false;
    memset(runtime, 0, sizeof(*runtime));
    TmcRaAdapter_Init(&runtime->adapter);
    if (nra_create(&params, &runtime->context) != NRA_OK)
        return false;
    runtime->owner_thread = pthread_self();
    runtime->owner_thread_set = true;
    return true;
}

bool TmcRaRuntime_InitDefault(TmcRaRuntime* runtime) {
    static const NRA_PlatformVTable platform = {.now_ms = Now};

    if (!TmcRaRuntime_Init(runtime, &platform, NULL))
        return false;
    if (!sAtexitRegistered && atexit(ShutdownAtExit) == 0)
        sAtexitRegistered = true;
    return true;
}

bool TmcRaRuntime_Frame(TmcRaRuntime* runtime) {
    NRA_StatusSnapshot status;

    if (!IsOwner(runtime) || runtime->context == NULL || runtime->shutdown_started)
        return false;
    nra_do_frame(runtime->context);
    if (!runtime->identify_requested && nra_copy_status_snapshot(runtime->context, &status) && status.logged_in) {
        runtime->identify_requested = true;
        (void)nra_load_current_game(runtime->context);
    }
    return TmcRaAdapter_TakeResetRequest(&runtime->adapter);
}

void TmcRaRuntime_Idle(TmcRaRuntime* runtime) {
    if (IsOwner(runtime) && runtime->context != NULL && !runtime->shutdown_started)
        nra_idle(runtime->context);
}

void TmcRaRuntime_ResetCompleted(TmcRaRuntime* runtime) {
    if (IsOwner(runtime) && runtime->context != NULL && !runtime->shutdown_started)
        nra_notify_reset_completed(runtime->context);
}

void TmcRaRuntime_Shutdown(TmcRaRuntime* runtime) {
    if (!IsOwner(runtime) || runtime->shutdown_started)
        return;
    runtime->shutdown_started = true;
    nra_unload_game(runtime->context);
    nra_destroy(runtime->context);
    runtime->context = NULL;
}

bool TmcRaRuntime_IsInitialized(const TmcRaRuntime* runtime) {
    return runtime != NULL && runtime->context != NULL && !runtime->shutdown_started;
}
