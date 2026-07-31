#include "native_ra_internal.h"

#include "rc_consoles.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static uint32_t nra_read_memory(uint32_t address, uint8_t* buffer, uint32_t count, rc_client_t* client) {
    NRA_Context* context = rc_client_get_userdata(client);
    size_t available;
    bool read;

    if (context == NULL || buffer == NULL) {
        return 0;
    }
    pthread_mutex_lock(&context->mutex);
    if (context->destroyed || !context->has_memory || address > context->memory.size) {
        pthread_mutex_unlock(&context->mutex);
        return 0;
    }
    available = context->memory.size - address;
    if (count > available) {
        pthread_mutex_unlock(&context->mutex);
        return 0;
    }
    if (context->game.read_memory_snapshot != NULL) {
        read = context->game.read_memory_snapshot(context->game_userdata, address, buffer, count);
        pthread_mutex_unlock(&context->mutex);
        return read ? count : 0;
    }
    if (!context->memory.fully_validated || context->memory.data == NULL) {
        pthread_mutex_unlock(&context->mutex);
        return 0;
    }
    memcpy(buffer, context->memory.data + address, count);
    pthread_mutex_unlock(&context->mutex);
    return count;
}

static char* nra_strdup(const char* value) {
    size_t size;
    char* copy;

    if (value == NULL) {
        return NULL;
    }
    size = strlen(value) + 1;
    copy = malloc(size);
    if (copy != NULL) {
        memcpy(copy, value, size);
    }
    return copy;
}

static void nra_free_request(NRA_Request* request) {
    if (request->post_data != NULL) {
        memset(request->post_data, 0, strlen(request->post_data));
    }
    free(request->url);
    free(request->post_data);
    free(request->content_type);
    free(request->user_agent);
    memset(request, 0, sizeof(*request));
}

static void nra_log(NRA_Context* context, int level, const char* message) {
    if (context->platform.log_redacted != NULL) {
        context->platform.log_redacted(context->platform_userdata, level, message);
    }
}

static rc_clock_t nra_now_ms(const rc_client_t* client) {
    NRA_Context* context = rc_client_get_userdata(client);
    if (context != NULL && context->platform.now_ms != NULL) {
        return context->platform.now_ms(context->platform_userdata);
    }
    return (rc_clock_t)clock() * 1000u / CLOCKS_PER_SEC;
}

static void nra_copy_text(char* destination, size_t capacity, const char* source) {
    if (capacity == 0) return;
    if (source == NULL) source = "";
    snprintf(destination, capacity, "%s", source);
}

static void nra_copy_achievement(NRA_UIAchievement* destination, const rc_client_achievement_t* source) {
    memset(destination, 0, sizeof(*destination));
    if (source == NULL) return;
    destination->id = source->id;
    destination->points = source->points;
    destination->unlocked = source->unlocked != 0;
    nra_copy_text(destination->title, sizeof(destination->title), source->title);
    nra_copy_text(destination->description, sizeof(destination->description), source->description);
    nra_copy_text(destination->badge_key, sizeof(destination->badge_key), source->badge_name);
    nra_copy_text(destination->measured_progress, sizeof(destination->measured_progress), source->measured_progress);
}

static void nra_ui_changed(NRA_Context* context) {
    ++context->ui.generation;
    if (context->ui.generation == 0) ++context->ui.generation;
}

static void nra_clear_game_ui(NRA_Context* context) {
    NRA_UISnapshot retained = context->ui;
    memset(&context->ui, 0, sizeof(context->ui));
    context->ui.available = true;
    context->ui.version = NRA_UI_SNAPSHOT_VERSION;
    context->ui.generation = retained.generation;
    context->ui.mode = context->mode;
    context->ui.logged_in = retained.logged_in;
    context->ui.account_score = retained.account_score;
    memcpy(context->ui.account_name, retained.account_name, sizeof(context->ui.account_name));
    context->ui.connection = retained.connection;
    nra_ui_changed(context);
}

static void nra_clear_transient_ui(NRA_Context* context) {
    context->ui.challenge_active = false;
    context->ui.progress_active = false;
    context->ui.leaderboard_tracker_active = false;
    context->ui.recent_leaderboard_result = false;
    memset(&context->ui.challenge, 0, sizeof(context->ui.challenge));
    memset(&context->ui.progress, 0, sizeof(context->ui.progress));
    context->ui.leaderboard_tracker_id = 0;
    context->ui.leaderboard_tracker_value[0] = '\0';
    context->ui.recent_leaderboard_id = 0;
    context->ui.recent_leaderboard_rank = 0;
    context->ui.recent_leaderboard_score[0] = '\0';
}

static void nra_push_toast(NRA_Context* context, NRA_UIToastKind kind, const rc_client_achievement_t* achievement,
                           const rc_client_leaderboard_t* leaderboard) {
    NRA_UIToast* toast;
    if (context->ui.toast_count == NRA_UI_TOAST_MAX) {
        memmove(context->ui.toasts, context->ui.toasts + 1,
                sizeof(context->ui.toasts[0]) * (NRA_UI_TOAST_MAX - 1));
        --context->ui.toast_count; /* Evict oldest; newest event remains visible. */
    }
    toast = &context->ui.toasts[context->ui.toast_count++];
    memset(toast, 0, sizeof(*toast));
    toast->sequence = ++context->next_toast_sequence;
    if (toast->sequence == 0)
        toast->sequence = ++context->next_toast_sequence;
    toast->kind = kind;
    if (achievement != NULL) {
        toast->related_id = achievement->id;
        toast->points = achievement->points;
        nra_copy_text(toast->title, sizeof(toast->title), achievement->title);
        nra_copy_text(toast->description, sizeof(toast->description), achievement->description);
        nra_copy_text(toast->badge_key, sizeof(toast->badge_key), achievement->badge_name);
        nra_copy_text(toast->measured_progress, sizeof(toast->measured_progress), achievement->measured_progress);
    } else if (leaderboard != NULL) {
        toast->related_id = leaderboard->id;
        nra_copy_text(toast->title, sizeof(toast->title), leaderboard->title);
        nra_copy_text(toast->description, sizeof(toast->description), leaderboard->description);
    }
}

void nra_handle_event(NRA_Context* context, const rc_client_event_t* event) {
    bool log_unknown = false;

    if (context == NULL || event == NULL) {
        return;
    }
    pthread_mutex_lock(&context->mutex);
    if (context->destroyed) {
        pthread_mutex_unlock(&context->mutex);
        return;
    }
    switch (event->type) {
    case RC_CLIENT_EVENT_ACHIEVEMENT_TRIGGERED:
        nra_push_toast(context, NRA_UI_TOAST_ACHIEVEMENT_UNLOCKED, event->achievement, NULL);
        context->achievement_list_dirty = true;
        break;
    case RC_CLIENT_EVENT_LEADERBOARD_STARTED:
        nra_push_toast(context, NRA_UI_TOAST_LEADERBOARD_STARTED, NULL, event->leaderboard);
        break;
    case RC_CLIENT_EVENT_LEADERBOARD_FAILED:
        nra_push_toast(context, NRA_UI_TOAST_LEADERBOARD_FAILED, NULL, event->leaderboard);
        break;
    case RC_CLIENT_EVENT_LEADERBOARD_SUBMITTED:
        nra_push_toast(context, NRA_UI_TOAST_LEADERBOARD_SUBMITTED, NULL, event->leaderboard);
        break;
    case RC_CLIENT_EVENT_ACHIEVEMENT_CHALLENGE_INDICATOR_SHOW:
        context->ui.challenge_active = true;
        nra_copy_achievement(&context->ui.challenge, event->achievement);
        break;
    case RC_CLIENT_EVENT_ACHIEVEMENT_CHALLENGE_INDICATOR_HIDE:
        context->ui.challenge_active = false;
        memset(&context->ui.challenge, 0, sizeof(context->ui.challenge));
        break;
    case RC_CLIENT_EVENT_ACHIEVEMENT_PROGRESS_INDICATOR_SHOW:
        context->ui.progress_active = true;
        nra_copy_achievement(&context->ui.progress, event->achievement);
        break;
    case RC_CLIENT_EVENT_ACHIEVEMENT_PROGRESS_INDICATOR_HIDE:
        context->ui.progress_active = false;
        memset(&context->ui.progress, 0, sizeof(context->ui.progress));
        break;
    case RC_CLIENT_EVENT_ACHIEVEMENT_PROGRESS_INDICATOR_UPDATE:
        context->ui.progress_active = true;
        nra_copy_achievement(&context->ui.progress, event->achievement);
        break;
    case RC_CLIENT_EVENT_LEADERBOARD_TRACKER_SHOW:
    case RC_CLIENT_EVENT_LEADERBOARD_TRACKER_UPDATE:
        if (event->leaderboard_tracker != NULL) {
            context->ui.leaderboard_tracker_active = true;
            context->ui.leaderboard_tracker_id = event->leaderboard_tracker->id;
            nra_copy_text(context->ui.leaderboard_tracker_value, sizeof(context->ui.leaderboard_tracker_value),
                          event->leaderboard_tracker->display);
        }
        break;
    case RC_CLIENT_EVENT_LEADERBOARD_TRACKER_HIDE:
        context->ui.leaderboard_tracker_active = false;
        context->ui.leaderboard_tracker_id = 0;
        context->ui.leaderboard_tracker_value[0] = '\0';
        break;
    case RC_CLIENT_EVENT_LEADERBOARD_SCOREBOARD:
        if (event->leaderboard_scoreboard != NULL) {
            context->ui.recent_leaderboard_result = true;
            context->ui.recent_leaderboard_id = event->leaderboard_scoreboard->leaderboard_id;
            context->ui.recent_leaderboard_rank = event->leaderboard_scoreboard->new_rank;
            nra_copy_text(context->ui.recent_leaderboard_score, sizeof(context->ui.recent_leaderboard_score),
                          event->leaderboard_scoreboard->submitted_score);
        }
        break;
    case RC_CLIENT_EVENT_GAME_COMPLETED:
        nra_push_toast(context, NRA_UI_TOAST_GAME_COMPLETED, NULL, NULL);
        break;
    case RC_CLIENT_EVENT_SUBSET_COMPLETED:
        nra_push_toast(context, NRA_UI_TOAST_SUBSET_COMPLETED, NULL, NULL);
        break;
    case RC_CLIENT_EVENT_SERVER_ERROR:
        context->ui.has_error = true;
        nra_copy_text(context->ui.error, sizeof(context->ui.error), "RetroAchievements server request failed");
        nra_push_toast(context, NRA_UI_TOAST_ERROR, NULL, NULL);
        break;
    case RC_CLIENT_EVENT_DISCONNECTED:
        context->ui.connection = NRA_UI_CONNECTION_OFFLINE;
        nra_push_toast(context, NRA_UI_TOAST_DISCONNECTED, NULL, NULL);
        break;
    case RC_CLIENT_EVENT_RECONNECTED:
        context->ui.connection = NRA_UI_CONNECTION_ONLINE;
        nra_push_toast(context, NRA_UI_TOAST_RECONNECTED, NULL, NULL);
        break;
    case RC_CLIENT_EVENT_RESET:
        context->reset_pending = true;
        nra_clear_transient_ui(context);
        break;
    default:
        if (context->unknown_event_count++ == 0) {
            log_unknown = true;
        }
        break;
    }
    nra_ui_changed(context);
    pthread_mutex_unlock(&context->mutex);
    if (log_unknown) {
        nra_log(context, RC_CLIENT_LOG_LEVEL_WARN, "native_ra: unknown rcheevos event");
    }
}

static void nra_event_handler(const rc_client_event_t* event, rc_client_t* client) {
    nra_handle_event(rc_client_get_userdata(client), event);
}

static void nra_rebuild_achievement_list(NRA_Context* context) {
    rc_client_achievement_list_t* list;
    rc_client_user_game_summary_t summary = {0};
    uint32_t bucket, index;
    NRA_UISnapshot rebuilt;

    memset(&rebuilt, 0, sizeof(rebuilt));
    list = rc_client_create_achievement_list(context->client, RC_CLIENT_ACHIEVEMENT_CATEGORY_CORE,
                                             RC_CLIENT_ACHIEVEMENT_LIST_GROUPING_PROGRESS);
    if (list != NULL) {
        for (bucket = 0; bucket < list->num_buckets; ++bucket) {
            const rc_client_achievement_bucket_t* source_bucket = &list->buckets[bucket];
            for (index = 0; index < source_bucket->num_achievements; ++index) {
                const rc_client_achievement_t* achievement = source_bucket->achievements[index];
                if (achievement == NULL) continue;
                ++rebuilt.achievements_total;
                if (achievement->unlocked) ++rebuilt.achievements_unlocked;
                rebuilt.achievement_points_total += achievement->points;
                if (achievement->unlocked) rebuilt.achievement_points_unlocked += achievement->points;
                if (rebuilt.achievement_count < NRA_UI_ACHIEVEMENT_MAX)
                    nra_copy_achievement(&rebuilt.achievements[rebuilt.achievement_count++], achievement);
            }
        }
        rc_client_destroy_achievement_list(list);
    }
    rc_client_get_user_game_summary(context->client, &summary);
    if (summary.num_core_achievements != 0) {
        rebuilt.achievements_total = summary.num_core_achievements;
        rebuilt.achievements_unlocked = summary.num_unlocked_achievements;
        rebuilt.achievement_points_total = summary.points_core;
        rebuilt.achievement_points_unlocked = summary.points_unlocked;
    }
    pthread_mutex_lock(&context->mutex);
    memcpy(context->ui.achievements, rebuilt.achievements, sizeof(context->ui.achievements));
    context->ui.achievement_count = rebuilt.achievement_count;
    context->ui.achievements_total = rebuilt.achievements_total;
    context->ui.achievements_unlocked = rebuilt.achievements_unlocked;
    context->ui.achievement_points_total = rebuilt.achievement_points_total;
    context->ui.achievement_points_unlocked = rebuilt.achievement_points_unlocked;
    nra_ui_changed(context);
    pthread_mutex_unlock(&context->mutex);
}

static void nra_refresh_rich_presence(NRA_Context* context) {
    char presence[NRA_UI_RICH_PRESENCE_MAX] = "";
    if (rc_client_has_rich_presence(context->client))
        rc_client_get_rich_presence_message(context->client, presence, sizeof(presence));
    pthread_mutex_lock(&context->mutex);
    if (context->game_loaded && strcmp(context->ui.rich_presence, presence) != 0) {
        nra_copy_text(context->ui.rich_presence, sizeof(context->ui.rich_presence), presence);
        nra_ui_changed(context);
    }
    pthread_mutex_unlock(&context->mutex);
}

static void nra_refresh_ui_after_frame(NRA_Context* context) {
    bool rebuild;
    nra_refresh_rich_presence(context);
    pthread_mutex_lock(&context->mutex);
    rebuild = context->achievement_list_dirty && context->game_loaded;
    context->achievement_list_dirty = false;
    pthread_mutex_unlock(&context->mutex);
    if (rebuild) nra_rebuild_achievement_list(context);
}

static void nra_client_log(const char* message, const rc_client_t* client) {
    NRA_Context* context = rc_client_get_userdata(client);
    bool active = false;

    (void)message; /* rcheevos messages can include request data; do not forward them verbatim. */
    if (context != NULL) {
        pthread_mutex_lock(&context->mutex);
        active = !context->destroyed;
        pthread_mutex_unlock(&context->mutex);
    }
    if (active) {
        nra_log(context, RC_CLIENT_LOG_LEVEL_INFO, "native_ra: rcheevos activity");
    }
}

static void nra_server_call(const rc_api_request_t* request, rc_client_server_callback_t callback,
                            void* callback_data, rc_client_t* client) {
    NRA_Context* context = rc_client_get_userdata(client);
    NRA_Request prepared = {0};
    NRA_HttpRequest http_request;
    NRA_RequestId id;
    char* post_data;
    size_t i;

    if (context == NULL || request == NULL || callback == NULL) {
        rc_api_server_response_t response = {NULL, 0, RC_API_SERVER_RESPONSE_CLIENT_ERROR};
        callback(&response, callback_data);
        return;
    }
    prepared.url = nra_strdup(request->url);
    prepared.post_data = nra_strdup(request->post_data);
    prepared.content_type = nra_strdup(request->content_type);
    prepared.user_agent = nra_strdup(context->user_agent);
    if (prepared.url == NULL || prepared.user_agent == NULL ||
        (request->post_data != NULL && prepared.post_data == NULL) ||
        (request->content_type != NULL && prepared.content_type == NULL)) {
        rc_api_server_response_t response = {NULL, 0, RC_API_SERVER_RESPONSE_CLIENT_ERROR};
        nra_free_request(&prepared);
        callback(&response, callback_data);
        return;
    }
    prepared.callback = callback;
    prepared.callback_data = callback_data;

    pthread_mutex_lock(&context->mutex);
    if (context->destroying || context->destroyed) {
        pthread_mutex_unlock(&context->mutex);
        nra_free_request(&prepared);
        {
            rc_api_server_response_t response = {NULL, 0, RC_API_SERVER_RESPONSE_CLIENT_ERROR};
            callback(&response, callback_data);
        }
        return;
    }
    for (i = 0; i < NRA_MAX_IN_FLIGHT_REQUESTS; ++i) {
        if (!context->requests[i].used) {
            break;
        }
    }
    if (i == NRA_MAX_IN_FLIGHT_REQUESTS) {
        pthread_mutex_unlock(&context->mutex);
        rc_api_server_response_t response = {NULL, 0, RC_API_SERVER_RESPONSE_RETRYABLE_CLIENT_ERROR};
        nra_log(context, RC_CLIENT_LOG_LEVEL_WARN, "native_ra: request limit reached");
        nra_free_request(&prepared);
        callback(&response, callback_data);
        return;
    }
    prepared.id = ++context->next_request_id;
    if (prepared.id == 0) {
        prepared.id = ++context->next_request_id;
    }
    prepared.requires_memory = context->load_pending;
    prepared.used = true;
    context->requests[i] = prepared;
    id = prepared.id;
    http_request.url = prepared.url;
    http_request.post_data = prepared.post_data;
    http_request.content_type = prepared.content_type;
    http_request.user_agent = prepared.user_agent;
    pthread_mutex_unlock(&context->mutex);

    http_request.connect_timeout_ms = 10000;
    http_request.read_timeout_ms = 30000;
    http_request.max_response_bytes = NRA_MAX_RESPONSE_BYTES;
    if (context->platform.http_begin != NULL) {
        context->platform.http_begin(context->platform_userdata, id, &http_request);
    } else {
        NRA_Request failed = {0};
        rc_api_server_response_t response = {NULL, 0, RC_API_SERVER_RESPONSE_CLIENT_ERROR};
        pthread_mutex_lock(&context->mutex);
        for (i = 0; i < NRA_MAX_IN_FLIGHT_REQUESTS; ++i) {
            if (context->requests[i].used && context->requests[i].id == id) {
                failed = context->requests[i];
                memset(&context->requests[i], 0, sizeof(context->requests[i]));
                break;
            }
        }
        pthread_mutex_unlock(&context->mutex);
        nra_free_request(&failed);
        callback(&response, callback_data);
        return;
    }

    /* The platform contract copied it before returning; never retain credentials. */
    pthread_mutex_lock(&context->mutex);
    post_data = NULL;
    for (i = 0; i < NRA_MAX_IN_FLIGHT_REQUESTS; ++i) {
        if (context->requests[i].used && context->requests[i].id == id) {
            post_data = context->requests[i].post_data;
            context->requests[i].post_data = NULL;
            break;
        }
    }
    pthread_mutex_unlock(&context->mutex);
    if (post_data != NULL) {
        memset(post_data, 0, strlen(post_data));
        free(post_data);
    }
}

static bool nra_valid_context(NRA_Context* context) {
    bool valid;

    if (context == NULL) {
        return false;
    }
    pthread_mutex_lock(&context->mutex);
    valid = context->client != NULL && !context->destroying && !context->destroyed;
    pthread_mutex_unlock(&context->mutex);
    return valid;
}

static void nra_dispatch_pending_reset(NRA_Context* context) {
    bool reset_pending;

    pthread_mutex_lock(&context->mutex);
    reset_pending = context->reset_pending;
    context->reset_pending = false;
    pthread_mutex_unlock(&context->mutex);
    if (reset_pending) {
        if (context->game.request_full_reset != NULL) {
            context->game.request_full_reset(context->game_userdata, RC_CLIENT_EVENT_RESET);
        }
    }
}

static void nra_cancel_requests(NRA_Context* context) {
    NRA_RequestId ids[NRA_MAX_IN_FLIGHT_REQUESTS];
    NRA_Request requests[NRA_MAX_IN_FLIGHT_REQUESTS] = {{0}};
    NRA_Completion completions[NRA_MAX_COMPLETIONS] = {{0}};
    size_t count = 0;
    size_t completion_count = 0;
    size_t i;

    pthread_mutex_lock(&context->mutex);
    for (i = 0; i < NRA_MAX_IN_FLIGHT_REQUESTS; ++i) {
        if (context->requests[i].used) {
            ids[count++] = context->requests[i].id;
            requests[i] = context->requests[i];
            memset(&context->requests[i], 0, sizeof(NRA_Request));
        }
    }
    for (i = 0; i < context->completion_count; ++i) {
        NRA_Completion* completion = &context->completions[(context->completion_head + i) % NRA_MAX_COMPLETIONS];
        completions[completion_count++] = *completion;
        memset(completion, 0, sizeof(*completion));
    }
    context->completion_head = 0;
    context->completion_count = 0;
    pthread_mutex_unlock(&context->mutex);
    for (i = 0; i < count; ++i) {
        if (context->platform.http_cancel != NULL) {
            context->platform.http_cancel(context->platform_userdata, ids[i]);
        }
    }
    for (i = 0; i < NRA_MAX_IN_FLIGHT_REQUESTS; ++i) {
        if (requests[i].callback != NULL) {
            rc_api_server_response_t response = {NULL, 0, RC_API_SERVER_RESPONSE_CLIENT_ERROR};
            requests[i].callback(&response, requests[i].callback_data);
        }
        nra_free_request(&requests[i]);
    }
    for (i = 0; i < completion_count; ++i) {
        free(completions[i].body);
    }
}

static void nra_drain_completions(NRA_Context* context, bool allow_memory_requests) {
    for (;;) {
        NRA_Completion completion;
        NRA_Request request = {0};
        size_t i;
        size_t index;

        pthread_mutex_lock(&context->mutex);
        for (i = 0; i < context->completion_count; ++i) {
            index = (context->completion_head + i) % NRA_MAX_COMPLETIONS;
            if (allow_memory_requests || !context->completions[index].requires_memory) {
                break;
            }
        }
        if (i == context->completion_count) {
            pthread_mutex_unlock(&context->mutex);
            break;
        }
        completion = context->completions[index];
        while (i + 1 < context->completion_count) {
            size_t next = (index + 1) % NRA_MAX_COMPLETIONS;
            context->completions[index] = context->completions[next];
            index = next;
            ++i;
        }
        index = (context->completion_head + context->completion_count - 1) % NRA_MAX_COMPLETIONS;
        memset(&context->completions[index], 0, sizeof(NRA_Completion));
        --context->completion_count;
        for (i = 0; i < NRA_MAX_IN_FLIGHT_REQUESTS; ++i) {
            if (context->requests[i].used && context->requests[i].id == completion.id) {
                request = context->requests[i];
                memset(&context->requests[i], 0, sizeof(NRA_Request));
                break;
            }
        }
        pthread_mutex_unlock(&context->mutex);
        if (request.callback != NULL) {
            rc_api_server_response_t response;
            response.body = (const char*)completion.body;
            response.body_length = completion.body_size;
            response.http_status_code = completion.retryable ? RC_API_SERVER_RESPONSE_RETRYABLE_CLIENT_ERROR :
                completion.http_status_code;
            request.callback(&response, request.callback_data);
        }
        nra_free_request(&request);
        free(completion.body);
    }
}

static void nra_login_callback(int result, const char* error_message, rc_client_t* client, void* userdata) {
    NRA_Context* context = userdata;
    const rc_client_user_t* user;
    bool persist_credentials;
    bool logged_in;
    NRA_Result stored_username;
    NRA_Result stored_token;
    (void)error_message;

    if (context == NULL) {
        return;
    }
    user = result == RC_OK ? rc_client_get_user_info(client) : NULL;
    pthread_mutex_lock(&context->mutex);
    if (context->destroyed || client != context->client) {
        pthread_mutex_unlock(&context->mutex);
        return;
    }
    context->logged_in = result == RC_OK;
    context->login_pending = false;
    persist_credentials = context->persist_login_credentials;
    context->persist_login_credentials = false;
    logged_in = context->logged_in;
    context->ui.logged_in = logged_in;
    context->ui.account_score = user != NULL ? user->score : 0;
    nra_copy_text(context->ui.account_name, sizeof(context->ui.account_name),
                  user != NULL ? user->display_name : NULL);
    if (logged_in) context->ui.connection = NRA_UI_CONNECTION_ONLINE;
    nra_ui_changed(context);
    pthread_mutex_unlock(&context->mutex);
    if (!persist_credentials) {
        if (result != RC_OK && result != RC_ABORTED && context->platform.secret_delete != NULL) {
            (void)context->platform.secret_delete(context->platform_userdata, "native_ra.username");
            (void)context->platform.secret_delete(context->platform_userdata, "native_ra.token");
        }
        return;
    }
    if (result != RC_OK) {
        return;
    }
    stored_username = user != NULL && user->username != NULL && user->token != NULL &&
        context->platform.secret_set != NULL
        ? context->platform.secret_set(context->platform_userdata, "native_ra.username",
                                       (const uint8_t*)user->username, strlen(user->username))
        : NRA_DISABLED;
    stored_token = stored_username == NRA_OK
        ? context->platform.secret_set(context->platform_userdata, "native_ra.token",
                                       (const uint8_t*)user->token, strlen(user->token))
        : stored_username;
    if (stored_token != NRA_OK) {
        if (context->platform.secret_delete != NULL) {
            (void)context->platform.secret_delete(context->platform_userdata, "native_ra.username");
            (void)context->platform.secret_delete(context->platform_userdata, "native_ra.token");
        }
        pthread_mutex_lock(&context->mutex);
        if (!context->destroyed && client == context->client) {
            context->ui.has_error = true;
            nra_copy_text(context->ui.error, sizeof(context->ui.error),
                          "RetroAchievements credentials were not saved");
            nra_ui_changed(context);
        }
        pthread_mutex_unlock(&context->mutex);
    }
}

static void nra_load_callback(int result, const char* error_message, rc_client_t* client, void* userdata) {
    NRA_Context* context = userdata;
    const rc_client_game_t* game;
    bool game_loaded;
    (void)error_message;

    if (context == NULL) {
        return;
    }
    pthread_mutex_lock(&context->mutex);
    if (context->destroyed || client != context->client) {
        pthread_mutex_unlock(&context->mutex);
        return;
    }
    pthread_mutex_unlock(&context->mutex);
    game_loaded = result == RC_OK && rc_client_is_game_loaded(client);
    pthread_mutex_lock(&context->mutex);
    if (context->destroyed || client != context->client) {
        pthread_mutex_unlock(&context->mutex);
        return;
    }
    context->load_pending = false;
    context->game_loaded = game_loaded;
    game_loaded = context->game_loaded;
    if (!game_loaded) nra_clear_game_ui(context);
    pthread_mutex_unlock(&context->mutex);
    game = game_loaded ? rc_client_get_game_info(client) : NULL;
    if (game_loaded && game != NULL && context->game.accept_identified_game != NULL &&
        !context->game.accept_identified_game(context->game_userdata, game->id, game->console_id)) {
        rc_client_unload_game(client);
        pthread_mutex_lock(&context->mutex);
        context->game_loaded = false;
        nra_clear_game_ui(context);
        pthread_mutex_unlock(&context->mutex);
        return;
    }
    if (game_loaded && game != NULL) {
        pthread_mutex_lock(&context->mutex);
        context->ui.game_loaded = true;
        context->ui.game_supported = true;
        context->ui.game_id = game->id;
        nra_copy_text(context->ui.game_title, sizeof(context->ui.game_title), game->title);
        nra_ui_changed(context);
        pthread_mutex_unlock(&context->mutex);
        nra_rebuild_achievement_list(context);
        nra_refresh_rich_presence(context);
    }
    if (game_loaded && game != NULL && context->game.on_ra_game_loaded != NULL) {
        context->game.on_ra_game_loaded(context->game_userdata, game->id);
    }
}

NRA_Result nra_create(const NRA_CreateParams* params, NRA_Context** context) {
    NRA_Context* result;

    if (context == NULL) {
        return NRA_INVALID_ARGUMENT;
    }
    *context = NULL;
    if (params == NULL || params->abi_version != NRA_ABI_VERSION || params->client_name == NULL ||
        params->client_version == NULL || params->user_agent == NULL || params->platform == NULL ||
        params->game == NULL ||
        (params->platform->http_begin != NULL && params->platform->http_shutdown == NULL) ||
        (params->game->build_memory_snapshot != NULL && params->game->release_memory_snapshot == NULL)) {
        return NRA_INVALID_ARGUMENT;
    }

    result = calloc(1, sizeof(*result));
    if (result == NULL) {
        return NRA_INTERNAL_ERROR;
    }
    result->platform = *params->platform;
    result->platform_userdata = params->platform_userdata;
    result->game = *params->game;
    result->game_userdata = params->game_userdata;
    result->user_agent = nra_strdup(params->user_agent);
    if (result->user_agent == NULL) {
        free(result);
        return NRA_INTERNAL_ERROR;
    }
    result->mode = NRA_MODE_SPECTATOR;
    result->ui.available = true;
    result->ui.version = NRA_UI_SNAPSHOT_VERSION;
    result->ui.generation = 1;
    result->ui.mode = result->mode;
    result->ui.connection = NRA_UI_CONNECTION_UNKNOWN;
    result->client = rc_client_create(nra_read_memory, nra_server_call);
    if (result->client == NULL) {
        free(result->user_agent);
        free(result);
        return NRA_INTERNAL_ERROR;
    }
    if (pthread_mutex_init(&result->mutex, NULL) != 0) {
        rc_client_destroy(result->client);
        free(result->user_agent);
        free(result);
        return NRA_INTERNAL_ERROR;
    }
    rc_client_set_userdata(result->client, result);
    rc_client_set_event_handler(result->client, nra_event_handler);
    rc_client_enable_logging(result->client, RC_CLIENT_LOG_LEVEL_INFO, nra_client_log);
    rc_client_set_get_time_millisecs_function(result->client, nra_now_ms);
    rc_client_set_allow_background_memory_reads(result->client, 0);
    rc_client_set_hardcore_enabled(result->client, 0);
    rc_client_set_spectator_mode_enabled(result->client, 1);
    *context = result;
    return NRA_OK;
}

void nra_destroy(NRA_Context* context) {
    if (context == NULL) {
        return;
    }
    pthread_mutex_lock(&context->mutex);
    if (context->destroying || context->destroyed) {
        pthread_mutex_unlock(&context->mutex);
        return;
    }
    context->destroying = true;
    pthread_mutex_unlock(&context->mutex);
    /*
     * rc_client_destroy marks its async callback data destroyed. Delivering the
     * saved server callbacks afterward lets rcheevos release that data without
     * touching its freed client mutex.
     */
    rc_client_destroy(context->client);
    nra_cancel_requests(context);
    if (context->platform.http_shutdown != NULL) {
        context->platform.http_shutdown(context->platform_userdata);
    }
    pthread_mutex_lock(&context->mutex);
    context->destroyed = true;
    context->destroying = false;
    context->client = NULL;
    pthread_mutex_unlock(&context->mutex);
    free(context->user_agent);
    pthread_mutex_destroy(&context->mutex);
    free(context);
}

NRA_Result nra_login_password(NRA_Context* context, const char* username, const char* password) {
    if (!nra_valid_context(context) || username == NULL || password == NULL || username[0] == '\0' || password[0] == '\0') {
        return NRA_INVALID_ARGUMENT;
    }
    pthread_mutex_lock(&context->mutex);
    if (context->login_pending) {
        pthread_mutex_unlock(&context->mutex);
        return NRA_INVALID_STATE;
    }
    context->login_pending = true;
    context->persist_login_credentials = true;
    pthread_mutex_unlock(&context->mutex);
    if (rc_client_begin_login_with_password(context->client, username, password, nra_login_callback, context) == NULL) {
        pthread_mutex_lock(&context->mutex);
        context->login_pending = false;
        context->persist_login_credentials = false;
        pthread_mutex_unlock(&context->mutex);
        return NRA_INTERNAL_ERROR;
    }
    return NRA_PENDING;
}

NRA_Result nra_login_token(NRA_Context* context, const char* username, const char* token) {
    if (!nra_valid_context(context) || username == NULL || token == NULL || username[0] == '\0' || token[0] == '\0') {
        return NRA_INVALID_ARGUMENT;
    }
    pthread_mutex_lock(&context->mutex);
    if (context->login_pending) {
        pthread_mutex_unlock(&context->mutex);
        return NRA_INVALID_STATE;
    }
    context->login_pending = true;
    context->persist_login_credentials = false;
    pthread_mutex_unlock(&context->mutex);
    if (rc_client_begin_login_with_token(context->client, username, token, nra_login_callback, context) == NULL) {
        pthread_mutex_lock(&context->mutex);
        context->login_pending = false;
        pthread_mutex_unlock(&context->mutex);
        return NRA_INTERNAL_ERROR;
    }
    return NRA_PENDING;
}

void nra_logout(NRA_Context* context, bool delete_persisted_credentials) {
    if (nra_valid_context(context)) {
        bool game_was_loaded;

        pthread_mutex_lock(&context->mutex);
        game_was_loaded = context->game_loaded || context->load_pending;
        context->logged_in = false;
        context->login_pending = false;
        context->game_loaded = false;
        context->load_pending = false;
        memset(&context->ui, 0, sizeof(context->ui));
        context->ui.available = true;
        context->ui.version = NRA_UI_SNAPSHOT_VERSION;
        context->ui.generation = 1;
        context->ui.mode = context->mode;
        context->ui.connection = NRA_UI_CONNECTION_UNKNOWN;
        pthread_mutex_unlock(&context->mutex);
        rc_client_logout(context->client);
        nra_cancel_requests(context);
        if (game_was_loaded && context->game.on_ra_game_unloaded != NULL) {
            context->game.on_ra_game_unloaded(context->game_userdata);
        }
        if (delete_persisted_credentials) {
            if (context->platform.secret_delete != NULL) {
                (void)context->platform.secret_delete(context->platform_userdata, "native_ra.username");
                (void)context->platform.secret_delete(context->platform_userdata, "native_ra.token");
            }
        }
    }
}

NRA_Result nra_load_current_game(NRA_Context* context) {
    NRA_Mode mode;
    if (!nra_valid_context(context)) {
        return NRA_INVALID_ARGUMENT;
    }
    NRA_RomView rom = {0};

    pthread_mutex_lock(&context->mutex);
    if (context->game_loaded || context->load_pending) {
        pthread_mutex_unlock(&context->mutex);
        return NRA_INVALID_STATE;
    }
    mode = context->mode;
    pthread_mutex_unlock(&context->mutex);
    if (context->game.get_rom == NULL || context->game.get_rom(context->game_userdata, &rom) != NRA_OK ||
        rom.data == NULL || rom.size == 0) {
        return NRA_INVALID_STATE;
    }
    rc_client_set_hardcore_enabled(context->client, 0);
    rc_client_set_spectator_mode_enabled(context->client, mode == NRA_MODE_SPECTATOR);
    pthread_mutex_lock(&context->mutex);
    if (context->game_loaded || context->load_pending) {
        pthread_mutex_unlock(&context->mutex);
        return NRA_INVALID_STATE;
    }
    context->load_pending = true;
    pthread_mutex_unlock(&context->mutex);
    if (rc_client_begin_identify_and_load_game(context->client, RC_CONSOLE_GAMEBOY_ADVANCE, rom.path_hint,
                                                rom.data, rom.size, nra_load_callback, context) == NULL) {
        pthread_mutex_lock(&context->mutex);
        context->load_pending = false;
        pthread_mutex_unlock(&context->mutex);
        return NRA_INTERNAL_ERROR;
    }
    return NRA_PENDING;
}

void nra_unload_game(NRA_Context* context) {
    if (!nra_valid_context(context)) {
        return;
    }
    pthread_mutex_lock(&context->mutex);
    if (!context->game_loaded && !context->load_pending) {
        pthread_mutex_unlock(&context->mutex);
        return;
    }
    context->game_loaded = false;
    context->load_pending = false;
    context->achievement_list_dirty = false;
    nra_clear_game_ui(context);
    pthread_mutex_unlock(&context->mutex);
    rc_client_unload_game(context->client);
    nra_cancel_requests(context);
    if (context->game.on_ra_game_unloaded != NULL) {
        context->game.on_ra_game_unloaded(context->game_userdata);
    }
}

void nra_do_frame(NRA_Context* context) {
    NRA_MemoryView memory = {0};
    bool acquired = false;
    bool processed = false;
    bool need_memory;

    if (!nra_valid_context(context)) {
        return;
    }
    nra_drain_completions(context, false);
    pthread_mutex_lock(&context->mutex);
    need_memory = context->load_pending || context->game_loaded;
    pthread_mutex_unlock(&context->mutex);
    if (need_memory && context->game.build_memory_snapshot != NULL &&
        context->game.build_memory_snapshot(context->game_userdata, &memory) == NRA_OK) {
        acquired = true;
    }
    if (acquired && memory.data != NULL &&
        (memory.fully_validated || context->game.read_memory_snapshot != NULL) &&
        (context->game.is_game_tick_processable == NULL || context->game.is_game_tick_processable(context->game_userdata))) {
        pthread_mutex_lock(&context->mutex);
        context->memory = memory;
        context->has_memory = true;
        pthread_mutex_unlock(&context->mutex);
        nra_drain_completions(context, true);
        rc_client_do_frame(context->client);
        processed = true;
        pthread_mutex_lock(&context->mutex);
        context->has_memory = false;
        memset(&context->memory, 0, sizeof(context->memory));
        pthread_mutex_unlock(&context->mutex);
    }
    if (acquired && context->game.release_memory_snapshot != NULL) {
        context->game.release_memory_snapshot(context->game_userdata, &memory);
    }
    if (processed) {
        nra_dispatch_pending_reset(context);
        nra_refresh_ui_after_frame(context);
    }
}

void nra_idle(NRA_Context* context) {
    if (nra_valid_context(context)) {
        bool load_pending;

        nra_drain_completions(context, false);
        pthread_mutex_lock(&context->mutex);
        load_pending = context->load_pending;
        pthread_mutex_unlock(&context->mutex);
        if (!load_pending) {
            rc_client_idle(context->client);
        }
    }
}

void nra_notify_reset_completed(NRA_Context* context) {
    bool game_loaded;

    if (!nra_valid_context(context)) {
        return;
    }
    pthread_mutex_lock(&context->mutex);
    game_loaded = context->game_loaded;
    pthread_mutex_unlock(&context->mutex);
    if (game_loaded) {
        rc_client_reset(context->client);
    }
}

NRA_Result nra_request_mode(NRA_Context* context, NRA_Mode mode) {
    NRA_CapabilityPolicy policy = {0};

    if (!nra_valid_context(context) || mode < NRA_MODE_SPECTATOR || mode > NRA_MODE_HARDCORE_APPROVED) {
        return NRA_INVALID_ARGUMENT;
    }
    if (mode == NRA_MODE_STRICT_UNAPPROVED && !NRA_ENABLE_STRICT_MODE) {
        return NRA_DISABLED;
    }
    if (mode == NRA_MODE_HARDCORE_APPROVED && !NRA_ENABLE_HARDCORE_APPROVED) {
        return NRA_DISABLED;
    }
    pthread_mutex_lock(&context->mutex);
    if (context->game_loaded && ((context->mode == NRA_MODE_SPECTATOR) != (mode == NRA_MODE_SPECTATOR))) {
        pthread_mutex_unlock(&context->mutex);
        return NRA_INVALID_STATE;
    }
    context->mode = mode;
    context->ui.mode = mode;
    nra_ui_changed(context);
    pthread_mutex_unlock(&context->mutex);
    policy.spectator_mode = mode == NRA_MODE_SPECTATOR;
    policy.live_casual_mode = mode == NRA_MODE_LIVE_CASUAL;
    policy.strict_mode = mode == NRA_MODE_STRICT_UNAPPROVED;
    if (context->game.apply_capability_policy != NULL) {
        context->game.apply_capability_policy(context->game_userdata, &policy);
    }
    return NRA_OK;
}

bool nra_can_pause(NRA_Context* context, uint32_t* frames_remaining) {
    if (!nra_valid_context(context)) {
        return false;
    }
    return rc_client_can_pause(context->client, frames_remaining) != 0;
}

size_t nra_progress_size(NRA_Context* context) {
    return nra_valid_context(context) ? rc_client_progress_size(context->client) : 0;
}

NRA_Result nra_serialize_progress(NRA_Context* context, uint8_t* buffer, size_t capacity, size_t* size) {
    size_t required;
    if (!nra_valid_context(context) || size == NULL) {
        return NRA_INVALID_ARGUMENT;
    }
    required = rc_client_progress_size(context->client);
    *size = required;
    if (buffer == NULL || capacity < required) {
        return NRA_INVALID_ARGUMENT;
    }
    return rc_client_serialize_progress_sized(context->client, buffer, capacity) == RC_OK ? NRA_OK : NRA_INTERNAL_ERROR;
}

NRA_Result nra_deserialize_progress(NRA_Context* context, const uint8_t* buffer, size_t size) {
    if (!nra_valid_context(context) || buffer == NULL || size == 0) {
        return NRA_INVALID_ARGUMENT;
    }
    return rc_client_deserialize_progress_sized(context->client, buffer, size) == RC_OK ? NRA_OK : NRA_INTERNAL_ERROR;
}

NRA_Result nra_reset_progress(NRA_Context* context) {
    if (!nra_valid_context(context)) {
        return NRA_INVALID_ARGUMENT;
    }
    return rc_client_deserialize_progress_sized(context->client, NULL, 0) == RC_OK ? NRA_OK : NRA_INTERNAL_ERROR;
}

NRA_Result nra_enqueue_http_completion(NRA_Context* context, const NRA_HttpCompletion* completion) {
    NRA_Completion prepared = {0};
    NRA_Completion* slot;
    size_t i;
    size_t request_index;

    if (context == NULL || completion == NULL || (completion->body == NULL && completion->body_size != 0)) {
        return NRA_INVALID_ARGUMENT;
    }
    if (completion->body_size > NRA_MAX_RESPONSE_BYTES) {
        return NRA_INVALID_ARGUMENT;
    }
    if (completion->body_size) {
        prepared.body = malloc(completion->body_size + 1);
        if (prepared.body == NULL) {
            return NRA_INTERNAL_ERROR;
        }
        memcpy(prepared.body, completion->body, completion->body_size);
        prepared.body[completion->body_size] = '\0';
    }
    pthread_mutex_lock(&context->mutex);
    if (context->destroying || context->destroyed) {
        pthread_mutex_unlock(&context->mutex);
        free(prepared.body);
        return NRA_DISABLED;
    }
    if (context->completion_count == NRA_MAX_COMPLETIONS) {
        pthread_mutex_unlock(&context->mutex);
        free(prepared.body);
        return NRA_DISABLED;
    }
    for (i = 0; i < NRA_MAX_IN_FLIGHT_REQUESTS; ++i) {
        if (context->requests[i].used && context->requests[i].id == completion->request_id) {
            break;
        }
        if (i + 1 == NRA_MAX_IN_FLIGHT_REQUESTS) {
            pthread_mutex_unlock(&context->mutex);
            free(prepared.body);
            return NRA_INVALID_STATE;
        }
    }
    request_index = i;
    for (size_t i = 0; i < context->completion_count; ++i) {
        NRA_Completion* queued = &context->completions[
            (context->completion_head + i) % NRA_MAX_COMPLETIONS];
        if (queued->id == completion->request_id) {
            pthread_mutex_unlock(&context->mutex);
            free(prepared.body);
            return NRA_INVALID_STATE;
        }
    }
    slot = &context->completions[(context->completion_head + context->completion_count) % NRA_MAX_COMPLETIONS];
    *slot = prepared;
    slot->id = completion->request_id;
    slot->http_status_code = completion->http_status_code;
    slot->body_size = completion->body_size;
    slot->retryable = completion->retryable;
    slot->requires_memory = context->requests[request_index].requires_memory;
    ++context->completion_count;
    pthread_mutex_unlock(&context->mutex);
    return NRA_OK;
}

bool nra_copy_status_snapshot(NRA_Context* context, NRA_StatusSnapshot* snapshot) {
    size_t i;

    if (!nra_valid_context(context) || snapshot == NULL) {
        return false;
    }
    pthread_mutex_lock(&context->mutex);
    snapshot->mode = context->mode;
    snapshot->client_created = true;
    snapshot->game_loaded = context->game_loaded;
    snapshot->request_pending = context->completion_count != 0;
    for (i = 0; i < NRA_MAX_IN_FLIGHT_REQUESTS; ++i) {
        if (context->requests[i].used) {
            snapshot->request_pending = true;
            break;
        }
    }
    snapshot->logged_in = context->logged_in;
    snapshot->load_pending = context->load_pending;
    pthread_mutex_unlock(&context->mutex);
    return true;
}

bool nra_copy_ui_snapshot(NRA_Context* context, NRA_UISnapshot* snapshot) {
    if (!nra_valid_context(context) || snapshot == NULL) {
        return false;
    }
    pthread_mutex_lock(&context->mutex);
    *snapshot = context->ui;
    pthread_mutex_unlock(&context->mutex);
    return true;
}
