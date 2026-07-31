#ifndef NATIVE_RA_OUTBOX_H
#define NATIVE_RA_OUTBOX_H

typedef enum NRA_OutboxKind {
    NRA_OUTBOX_NONE,
    NRA_OUTBOX_ACHIEVEMENT,
    NRA_OUTBOX_LEADERBOARD,
} NRA_OutboxKind;

NRA_OutboxKind nra_outbox_classify_request(const char* url, const char* post_data, const char* content_type);

#endif
