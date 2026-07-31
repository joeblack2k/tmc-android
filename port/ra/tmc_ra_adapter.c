#include "tmc_ra_adapter.h"

#include "port_rom.h"
#include "tmc_ra_memory.h"

#include "rc_consoles.h"

#include <assert.h>
#include <string.h>

static NRA_Result tmc_ra_get_rom(void* userdata, NRA_RomView* rom) {
    (void)userdata;
    if (rom == NULL || gRomData == NULL || gRomSize == 0)
        return NRA_DISABLED;
    rom->path_hint = Port_GetLoadedRomPath();
    rom->data = gRomData;
    rom->size = gRomSize;
    rom->console_id = RC_CONSOLE_GAMEBOY_ADVANCE;
    return NRA_OK;
}

static NRA_Result tmc_ra_build_memory_snapshot(void* userdata, NRA_MemoryView* memory) {
    TmcRaSnapshotView snapshot;
    (void)userdata;
    if (memory == NULL)
        return NRA_INVALID_ARGUMENT;

    TmcRaMemory_ResetAudit();
    TmcRaMemory_Publish();
    snapshot = TmcRaMemory_Current();
    memory->data = snapshot.bytes;
    memory->size = TMC_RA_SNAPSHOT_BYTES;
    memory->generation = snapshot.generation;
    /* Live remains gated until every address in this canonical view is proven. */
    memory->fully_validated = false;
    return NRA_OK;
}

static bool tmc_ra_read_memory_snapshot(void* userdata, uint32_t address, uint8_t* buffer, uint32_t count) {
    (void)userdata;
    return TmcRaMemory_ReadBlock(address, buffer, count, count);
}

static void tmc_ra_release_memory_snapshot(void* userdata, const NRA_MemoryView* memory) {
    (void)userdata;
    assert(memory == NULL || (memory->data != NULL && memory->size == TMC_RA_SNAPSHOT_BYTES));
}

static void tmc_ra_request_full_reset(void* userdata, uint32_t reason) {
    TmcRaAdapter* adapter = userdata;
    (void)reason;
    if (adapter != NULL)
        adapter->reset_requested = true;
}

static void tmc_ra_apply_capability_policy(void* userdata, const NRA_CapabilityPolicy* policy) {
    TmcRaAdapter* adapter = userdata;
    if (adapter != NULL)
        adapter->strict_mode = policy != NULL && policy->strict_mode;
}

static bool tmc_ra_accept_identified_game(void* userdata, uint32_t game_id, uint32_t console_id) {
    TmcRaAdapter* adapter = userdata;
    const bool accepted = game_id == 559 && console_id == RC_CONSOLE_GAMEBOY_ADVANCE;

    if (adapter != NULL) {
        adapter->admission = accepted ? TMC_RA_ADMISSION_SUPPORTED : TMC_RA_ADMISSION_UNSUPPORTED;
        adapter->identified_game_id = game_id;
    }
    return accepted;
}

void TmcRaAdapter_Init(TmcRaAdapter* adapter) {
    if (adapter != NULL) {
        TmcRaMemory_ResetRequestedCoverage();
        memset(adapter, 0, sizeof(*adapter));
    }
}

const NRA_GameAdapterVTable* TmcRaAdapter_VTable(void) {
    static const NRA_GameAdapterVTable vtable = {
        .get_rom = tmc_ra_get_rom,
        .build_memory_snapshot = tmc_ra_build_memory_snapshot,
        .read_memory_snapshot = tmc_ra_read_memory_snapshot,
        .release_memory_snapshot = tmc_ra_release_memory_snapshot,
        .request_full_reset = tmc_ra_request_full_reset,
        .apply_capability_policy = tmc_ra_apply_capability_policy,
        .accept_identified_game = tmc_ra_accept_identified_game,
    };
    return &vtable;
}

TmcRaAdmission TmcRaAdapter_Admission(const TmcRaAdapter* adapter) {
    return adapter ? adapter->admission : TMC_RA_ADMISSION_UNKNOWN;
}

bool TmcRaAdapter_TakeResetRequest(TmcRaAdapter* adapter) {
    const bool requested = adapter != NULL && adapter->reset_requested;
    if (adapter != NULL)
        adapter->reset_requested = false;
    return requested;
}

bool TmcRaAdapter_StrictMode(const TmcRaAdapter* adapter) {
    return adapter != NULL && adapter->strict_mode;
}
