#ifndef TMC_RA_ANDROID_QUEUE_H
#define TMC_RA_ANDROID_QUEUE_H

#include "native_ra/native_ra.h"

#include <pthread.h>

#define TMC_RA_ANDROID_QUEUE_CAPACITY 16u
#define TMC_RA_ANDROID_MAX_RESPONSE_BYTES (1024u * 1024u)
#define TMC_RA_ANDROID_MAX_USERNAME_BYTES 256u
#define TMC_RA_ANDROID_MAX_PASSWORD_BYTES 256u
#define TMC_RA_ANDROID_MAX_TOKEN_BYTES 1024u

typedef struct TmcRaAndroidCompletion {
    NRA_HttpCompletion completion;
    uint64_t generation;
    uint8_t* body;
} TmcRaAndroidCompletion;

typedef struct TmcRaAndroidLogin {
    uint64_t generation;
    char username[TMC_RA_ANDROID_MAX_USERNAME_BYTES + 1];
    char password[TMC_RA_ANDROID_MAX_PASSWORD_BYTES + 1];
    bool pending;
} TmcRaAndroidLogin;

typedef struct TmcRaAndroidQueue {
    pthread_mutex_t mutex;
    TmcRaAndroidCompletion completions[TMC_RA_ANDROID_QUEUE_CAPACITY];
    TmcRaAndroidLogin login;
    uint64_t generation;
    uint32_t head;
    uint32_t count;
    bool closed;
} TmcRaAndroidQueue;

bool TmcRaAndroidQueue_Init(TmcRaAndroidQueue* queue);
void TmcRaAndroidQueue_Destroy(TmcRaAndroidQueue* queue);
uint64_t TmcRaAndroidQueue_Generation(TmcRaAndroidQueue* queue);
uint64_t TmcRaAndroidQueue_AdvanceGeneration(TmcRaAndroidQueue* queue);
void TmcRaAndroidQueue_Close(TmcRaAndroidQueue* queue);

bool TmcRaAndroidQueue_EnqueueCompletion(TmcRaAndroidQueue* queue, uint64_t generation,
                                         const NRA_HttpCompletion* completion);
bool TmcRaAndroidQueue_TakeCompletion(TmcRaAndroidQueue* queue, uint64_t generation,
                                      TmcRaAndroidCompletion* completion);
void TmcRaAndroidQueue_ReleaseCompletion(TmcRaAndroidCompletion* completion);

bool TmcRaAndroidQueue_EnqueuePassword(TmcRaAndroidQueue* queue, uint64_t generation,
                                       const char* username, const char* password);
bool TmcRaAndroidQueue_TakePassword(TmcRaAndroidQueue* queue, uint64_t generation,
                                    TmcRaAndroidLogin* login);
void TmcRaAndroidQueue_Clear(TmcRaAndroidQueue* queue);

#endif
