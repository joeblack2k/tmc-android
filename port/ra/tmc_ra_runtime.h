#ifndef TMC_RA_RUNTIME_H
#define TMC_RA_RUNTIME_H

#include "native_ra/native_ra.h"
#include "tmc_ra_adapter.h"

#include <stdbool.h>
#include <pthread.h>

typedef struct TmcRaRuntime {
    NRA_Context* context;
    TmcRaAdapter adapter;
    pthread_t owner_thread;
    bool owner_thread_set;
    bool identify_requested;
    bool shutdown_started;
} TmcRaRuntime;

extern TmcRaRuntime gTmcRaRuntime;

uint64_t TmcRaRuntime_MonotonicMs(void);
bool TmcRaRuntime_Init(TmcRaRuntime* runtime, const NRA_PlatformVTable* platform, void* platform_userdata);
bool TmcRaRuntime_InitDefault(TmcRaRuntime* runtime);
bool TmcRaRuntime_Frame(TmcRaRuntime* runtime);
void TmcRaRuntime_Idle(TmcRaRuntime* runtime);
void TmcRaRuntime_ResetCompleted(TmcRaRuntime* runtime);
void TmcRaRuntime_Shutdown(TmcRaRuntime* runtime);
bool TmcRaRuntime_IsInitialized(const TmcRaRuntime* runtime);

#endif
