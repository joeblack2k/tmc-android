#include "tmc_ra_android_queue.h"

#include <stdio.h>
#include <string.h>

int main(void) {
    TmcRaAndroidQueue queue;
    TmcRaAndroidCompletion taken;
    TmcRaAndroidLogin login;
    const uint8_t body[] = {1, 2, 3};
    const NRA_HttpCompletion completion = {
        .request_id = 7,
        .http_status_code = 200,
        .body = body,
        .body_size = sizeof(body),
    };
    const uint64_t first_generation = 1;
    uint64_t second_generation;
    unsigned index;

    if (!TmcRaAndroidQueue_Init(&queue) || TmcRaAndroidQueue_Generation(&queue) != first_generation)
        return 1;
    for (index = 0; index < TMC_RA_ANDROID_QUEUE_CAPACITY; ++index) {
        NRA_HttpCompletion item = completion;
        item.request_id += index;
        if (!TmcRaAndroidQueue_EnqueueCompletion(&queue, first_generation, &item))
            return 1;
    }
    if (TmcRaAndroidQueue_EnqueueCompletion(&queue, first_generation, &completion))
        return 1;
    if (!TmcRaAndroidQueue_TakeCompletion(&queue, first_generation, &taken)
        || taken.completion.request_id != completion.request_id
        || taken.completion.body_size != sizeof(body)
        || memcmp(taken.completion.body, body, sizeof(body)) != 0)
        return 1;
    TmcRaAndroidQueue_ReleaseCompletion(&taken);
    if (!TmcRaAndroidQueue_EnqueuePassword(&queue, first_generation, "user", "password"))
        return 1;
    second_generation = TmcRaAndroidQueue_AdvanceGeneration(&queue);
    if (second_generation == first_generation || TmcRaAndroidQueue_TakePassword(&queue, first_generation, &login)
        || TmcRaAndroidQueue_EnqueueCompletion(&queue, first_generation, &completion))
        return 1;
    if (!TmcRaAndroidQueue_EnqueuePassword(&queue, second_generation, "user", "password")
        || !TmcRaAndroidQueue_TakePassword(&queue, second_generation, &login)
        || strcmp(login.username, "user") != 0 || strcmp(login.password, "password") != 0)
        return 1;
    memset(&login, 0, sizeof(login));
    TmcRaAndroidQueue_Close(&queue);
    if (TmcRaAndroidQueue_EnqueuePassword(&queue, second_generation, "user", "password")
        || TmcRaAndroidQueue_EnqueueCompletion(&queue, second_generation, &completion))
        return 1;
    TmcRaAndroidQueue_Destroy(&queue);
    puts("tmc_ra_android_queue_test: ALL PASS");
    return 0;
}
