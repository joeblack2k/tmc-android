#include "native_ra_outbox.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

static bool has_only_shape(const char* post, const char* const* allowed, size_t allowed_count,
                           const char* const* required, size_t required_count) {
    const char* item = post;
    size_t found = 0;
    size_t segments = 0;

    while (*item) {
        const char* end = strchr(item, '&');
        const char* equal = strchr(item, '=');
        size_t name_size;
        size_t value_size;
        size_t i;
        bool known = false;

        if (end == NULL) end = item + strlen(item);
        if (equal == NULL || equal >= end) return false;
        name_size = (size_t)(equal - item);
        value_size = (size_t)(end - equal - 1);
        for (i = 0; i < allowed_count; ++i) {
            if (strlen(allowed[i]) == name_size && memcmp(item, allowed[i], name_size) == 0) {
                known = true;
                break;
            }
        }
        if (!known || ++segments > allowed_count) return false;
        for (i = 0; i < required_count; ++i) {
            if (strlen(required[i]) == name_size && memcmp(item, required[i], name_size) == 0) {
                if (value_size == 0) return false;
                ++found;
                break;
            }
        }
        item = *end ? end + 1 : end;
    }
    return found == required_count;
}

NRA_OutboxKind nra_outbox_classify_request(const char* url, const char* post, const char* content_type) {
    static const char* const achievement_allowed[] = {"r", "u", "t", "a", "h", "m", "o", "v"};
    static const char* const achievement_required[] = {"r", "u", "t", "a", "h", "v"};
    static const char* const leaderboard_allowed[] = {"r", "u", "t", "i", "s", "m", "o", "v"};
    static const char* const leaderboard_required[] = {"r", "u", "t", "i", "s", "v"};

    if (url == NULL || post == NULL || content_type == NULL ||
        strcmp(url, "https://retroachievements.org/dorequest.php") != 0 ||
        strcmp(content_type, "application/x-www-form-urlencoded") != 0)
        return NRA_OUTBOX_NONE;
    if (strncmp(post, "r=awardachievement&", 19) == 0 &&
        has_only_shape(post, achievement_allowed, 8, achievement_required, 6))
        return NRA_OUTBOX_ACHIEVEMENT;
    if (strncmp(post, "r=submitlbentry&", 16) == 0 &&
        has_only_shape(post, leaderboard_allowed, 8, leaderboard_required, 6))
        return NRA_OUTBOX_LEADERBOARD;
    return NRA_OUTBOX_NONE;
}
