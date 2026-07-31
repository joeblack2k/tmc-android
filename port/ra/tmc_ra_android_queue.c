#include "tmc_ra_android_queue.h"

#include <stdlib.h>
#include <string.h>

static void Wipe(void* data, size_t size) {
    volatile uint8_t* bytes = data;

    while (size-- != 0)
        *bytes++ = 0;
}

static size_t BoundedLength(const char* value, size_t limit) {
    size_t size = 0;

    if (value == NULL)
        return limit + 1;
    while (size <= limit && value[size] != '\0')
        ++size;
    return size;
}

void TmcRaAndroidQueue_ReleaseCompletion(TmcRaAndroidCompletion* completion) {
    if (completion == NULL)
        return;
    if (completion->body != NULL) {
        Wipe(completion->body, completion->completion.body_size);
        free(completion->body);
    }
    memset(completion, 0, sizeof(*completion));
}

static void ClearLocked(TmcRaAndroidQueue* queue) {
    uint32_t index;

    while (queue->count != 0) {
        index = queue->head;
        TmcRaAndroidQueue_ReleaseCompletion(&queue->completions[index]);
        queue->head = (queue->head + 1) % TMC_RA_ANDROID_QUEUE_CAPACITY;
        --queue->count;
    }
    Wipe(queue->login.username, sizeof(queue->login.username));
    Wipe(queue->login.password, sizeof(queue->login.password));
    memset(&queue->login, 0, sizeof(queue->login));
}

bool TmcRaAndroidQueue_Init(TmcRaAndroidQueue* queue) {
    if (queue == NULL)
        return false;
    memset(queue, 0, sizeof(*queue));
    if (pthread_mutex_init(&queue->mutex, NULL) != 0)
        return false;
    queue->generation = 1;
    return true;
}

void TmcRaAndroidQueue_Destroy(TmcRaAndroidQueue* queue) {
    if (queue == NULL)
        return;
    pthread_mutex_lock(&queue->mutex);
    ClearLocked(queue);
    queue->closed = true;
    pthread_mutex_unlock(&queue->mutex);
    pthread_mutex_destroy(&queue->mutex);
}

uint64_t TmcRaAndroidQueue_Generation(TmcRaAndroidQueue* queue) {
    uint64_t generation;

    if (queue == NULL)
        return 0;
    pthread_mutex_lock(&queue->mutex);
    generation = queue->generation;
    pthread_mutex_unlock(&queue->mutex);
    return generation;
}

uint64_t TmcRaAndroidQueue_AdvanceGeneration(TmcRaAndroidQueue* queue) {
    uint64_t generation;

    if (queue == NULL)
        return 0;
    pthread_mutex_lock(&queue->mutex);
    ClearLocked(queue);
    generation = ++queue->generation;
    if (generation == 0)
        generation = ++queue->generation;
    pthread_mutex_unlock(&queue->mutex);
    return generation;
}

void TmcRaAndroidQueue_Close(TmcRaAndroidQueue* queue) {
    if (queue == NULL)
        return;
    pthread_mutex_lock(&queue->mutex);
    ClearLocked(queue);
    queue->closed = true;
    pthread_mutex_unlock(&queue->mutex);
}

bool TmcRaAndroidQueue_EnqueueCompletion(TmcRaAndroidQueue* queue, uint64_t generation,
                                         const NRA_HttpCompletion* completion) {
    TmcRaAndroidCompletion queued = {0};
    uint32_t index;

    if (queue == NULL || completion == NULL || (completion->body == NULL && completion->body_size != 0)
        || completion->body_size > TMC_RA_ANDROID_MAX_RESPONSE_BYTES)
        return false;
    if (completion->body_size != 0) {
        queued.body = malloc(completion->body_size);
        if (queued.body == NULL)
            return false;
        memcpy(queued.body, completion->body, completion->body_size);
    }
    queued.completion = *completion;
    queued.completion.body = queued.body;
    queued.generation = generation;

    pthread_mutex_lock(&queue->mutex);
    if (queue->closed || generation != queue->generation || queue->count == TMC_RA_ANDROID_QUEUE_CAPACITY) {
        pthread_mutex_unlock(&queue->mutex);
        TmcRaAndroidQueue_ReleaseCompletion(&queued);
        return false;
    }
    index = (queue->head + queue->count) % TMC_RA_ANDROID_QUEUE_CAPACITY;
    queue->completions[index] = queued;
    ++queue->count;
    pthread_mutex_unlock(&queue->mutex);
    return true;
}

bool TmcRaAndroidQueue_TakeCompletion(TmcRaAndroidQueue* queue, uint64_t generation,
                                      TmcRaAndroidCompletion* completion) {
    if (queue == NULL || completion == NULL)
        return false;
    memset(completion, 0, sizeof(*completion));
    pthread_mutex_lock(&queue->mutex);
    while (queue->count != 0 && queue->completions[queue->head].generation != generation) {
        TmcRaAndroidQueue_ReleaseCompletion(&queue->completions[queue->head]);
        queue->head = (queue->head + 1) % TMC_RA_ANDROID_QUEUE_CAPACITY;
        --queue->count;
    }
    if (queue->closed || generation != queue->generation || queue->count == 0) {
        pthread_mutex_unlock(&queue->mutex);
        return false;
    }
    *completion = queue->completions[queue->head];
    memset(&queue->completions[queue->head], 0, sizeof(queue->completions[queue->head]));
    queue->head = (queue->head + 1) % TMC_RA_ANDROID_QUEUE_CAPACITY;
    --queue->count;
    pthread_mutex_unlock(&queue->mutex);
    return true;
}

bool TmcRaAndroidQueue_EnqueuePassword(TmcRaAndroidQueue* queue, uint64_t generation,
                                       const char* username, const char* password) {
    const size_t username_size = BoundedLength(username, TMC_RA_ANDROID_MAX_USERNAME_BYTES);
    const size_t password_size = BoundedLength(password, TMC_RA_ANDROID_MAX_PASSWORD_BYTES);

    if (queue == NULL || username_size == 0 || password_size == 0
        || username_size > TMC_RA_ANDROID_MAX_USERNAME_BYTES
        || password_size > TMC_RA_ANDROID_MAX_PASSWORD_BYTES)
        return false;
    pthread_mutex_lock(&queue->mutex);
    if (queue->closed || generation != queue->generation || queue->login.pending) {
        pthread_mutex_unlock(&queue->mutex);
        return false;
    }
    memcpy(queue->login.username, username, username_size + 1);
    memcpy(queue->login.password, password, password_size + 1);
    queue->login.generation = generation;
    queue->login.pending = true;
    pthread_mutex_unlock(&queue->mutex);
    return true;
}

bool TmcRaAndroidQueue_TakePassword(TmcRaAndroidQueue* queue, uint64_t generation,
                                    TmcRaAndroidLogin* login) {
    if (queue == NULL || login == NULL)
        return false;
    memset(login, 0, sizeof(*login));
    pthread_mutex_lock(&queue->mutex);
    if (queue->closed || generation != queue->generation || !queue->login.pending
        || queue->login.generation != generation) {
        pthread_mutex_unlock(&queue->mutex);
        return false;
    }
    *login = queue->login;
    memset(&queue->login, 0, sizeof(queue->login));
    pthread_mutex_unlock(&queue->mutex);
    return true;
}

void TmcRaAndroidQueue_Clear(TmcRaAndroidQueue* queue) {
    if (queue == NULL)
        return;
    pthread_mutex_lock(&queue->mutex);
    ClearLocked(queue);
    pthread_mutex_unlock(&queue->mutex);
}
