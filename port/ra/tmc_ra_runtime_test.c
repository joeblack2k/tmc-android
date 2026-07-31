#define main tmc_ra_memory_test_main
#include "tmc_ra_memory_test.c"
#undef main

#include "tmc_ra_runtime.h"

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

typedef struct {
    TmcRaRuntime* runtime;
    unsigned requests;
    unsigned shutdowns;
    uint32_t generation_on_game_request;
} RuntimeFixture;

static const char kLoginResponse[] = "{\"Success\":true,\"User\":\"user\",\"Token\":\"token\"}";

const char* Port_GetLoadedRomPath(void) {
    return "synthetic.gba";
}

static uint64_t Now(void* userdata) {
    return ((RuntimeFixture*)userdata)->requests;
}

static void Begin(void* userdata, NRA_RequestId id, const NRA_HttpRequest* request) {
    RuntimeFixture* fixture = userdata;
    const NRA_HttpCompletion completion = {
        .request_id = id,
        .http_status_code = 200,
        .body = (const uint8_t*)kLoginResponse,
        .body_size = sizeof(kLoginResponse) - 1,
    };
    (void)request;
    ++fixture->requests;
    if (fixture->requests == 1)
        (void)nra_enqueue_http_completion(fixture->runtime->context, &completion);
    else if (fixture->requests == 2)
        fixture->generation_on_game_request = TmcRaMemory_Current().generation;
}

static void ShutdownHttp(void* userdata) {
    ++((RuntimeFixture*)userdata)->shutdowns;
}

static void* ForeignLifecycle(void* userdata) {
    TmcRaRuntime* runtime = userdata;

    if (TmcRaRuntime_Frame(runtime))
        return (void*)1;
    TmcRaRuntime_Idle(runtime);
    TmcRaRuntime_ResetCompleted(runtime);
    TmcRaRuntime_Shutdown(runtime);
    return NULL;
}

int main(void) {
    RuntimeFixture fixture = {0};
    TmcRaRuntime runtime = {0};
    pthread_t thread;
    void* result = NULL;
    uint8_t rom[] = { 0x12, 0x34, 0x56, 0x78 };
    uint32_t generation_before_frame;
    uint32_t generation;
    uint64_t monotonic_before;
    const struct timespec delay = {.tv_nsec = 20000000L};
    const NRA_PlatformVTable platform = {
        .now_ms = Now,
        .http_begin = Begin,
        .http_shutdown = ShutdownHttp,
    };

    if (tmc_ra_memory_test_main() != 0)
        return 1;
    monotonic_before = TmcRaRuntime_MonotonicMs();
    if (nanosleep(&delay, NULL) != 0 || TmcRaRuntime_MonotonicMs() < monotonic_before + 10)
        return 1;
    gRomData = rom;
    gRomSize = sizeof(rom);
    fixture.runtime = &runtime;
    if (!TmcRaRuntime_Init(&runtime, &platform, &fixture) || !TmcRaRuntime_IsInitialized(&runtime))
        return 1;
    if (nra_login_password(runtime.context, "user", "password") != NRA_PENDING)
        return 1;
    generation_before_frame = TmcRaMemory_Current().generation;
    if (pthread_create(&thread, NULL, ForeignLifecycle, &runtime) != 0)
        return 1;
    pthread_join(thread, &result);
    if (result != NULL || !TmcRaRuntime_IsInitialized(&runtime) || fixture.requests != 1)
        return 1;
    if (TmcRaRuntime_Frame(&runtime) || fixture.requests != 2
        || fixture.generation_on_game_request != generation_before_frame + 1)
        return 1;
    generation = TmcRaMemory_Current().generation;
    if (TmcRaRuntime_Frame(&runtime) || TmcRaMemory_Current().generation != generation + 1)
        return 1;
    TmcRaRuntime_ResetCompleted(&runtime);
    TmcRaRuntime_Shutdown(&runtime);
    if (TmcRaRuntime_IsInitialized(&runtime) || fixture.shutdowns != 1)
        return 1;
    if (!TmcRaRuntime_InitDefault(&runtime) || !TmcRaRuntime_IsInitialized(&runtime))
        return 1;
    TmcRaRuntime_Idle(&runtime);
    TmcRaRuntime_Shutdown(&runtime);
    if (TmcRaRuntime_IsInitialized(&runtime))
        return 1;
    if (!TmcRaRuntime_InitDefault(&gTmcRaRuntime) || !TmcRaRuntime_IsInitialized(&gTmcRaRuntime))
        return 1;
    puts("tmc_ra_runtime_test: ALL PASS");
    return 0;
}
