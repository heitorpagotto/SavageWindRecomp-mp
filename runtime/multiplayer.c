#ifdef SW_MULTIPLAYER
#include "multiplayer.h"

#include "game_sites.h"
#include "hle.h"
#include "random.h"
#include "settings.h"

#include <SDL3/SDL_clipboard.h>
#include <SDL3/SDL_stdinc.h>
#include <rtc/rtc.h>
#include <string.h>

typedef struct {
    SwmpM1Action action;
    char name[SWMP_PLAYER_NAME_SIZE];
} Command;

// The window thread publishes copies to the render and game threads.
static pthread_mutex_t view_lock = PTHREAD_MUTEX_INITIALIZER;
static SwmpM1View view;
static SwmpM1Roster roster;
static SwmpPosition local_position;
static Command command;
static bool stop_requested;

// Window thread only.
static SwmpCompatibility compatibility;
static SwmpDirect *direct;
static SwmpWebrtc *rtc;
static SwmpLobby lobby;
static uint64_t lobby_generation;
static SwmpConnection reply_connection;
static bool clipboard_error;

static bool read_build_id(uint8_t *out, size_t size, const char *hex) {
    static const char digits[] = "0123456789abcdef";
    if (!hex || strlen(hex) != 2 * size) return false;
    for (size_t i = 0; i < size; i++) {
        const char *hi = strchr(digits, hex[2 * i]), *lo = strchr(digits, hex[2 * i + 1]);
        if (!hi || !lo) return false;
        out[i] = (uint8_t)(((hi - digits) << 4) | (lo - digits));
    }
    return true;
}

void swmp_m1_boot(const GameLibTables *game) {
    compatibility = (SwmpCompatibility){
        .protocol = SWMP_PROTOCOL_VERSION, .mod_version = {0, 1, 0}, .game_version = {1, 9, 0},
    };
    // A gamelib ID also encodes local compiler/CPU flags. Use the common source identity for wire compatibility.
    bool supported = game && game->version && !strcmp(game->version, "1.9.0") && *game->nmodules &&
        g_site.mp_presence_tick && g_site.actor_index_assign && g_site.actor_index_resolve &&
        g_site.actor_index_release && g_site.actor_link_release;
    if (supported &&
        (!read_build_id(compatibility.game_build, sizeof compatibility.game_build, game->modules[0].build_id) ||
         !read_build_id(compatibility.savage_build, sizeof compatibility.savage_build, SWMP_RUNTIME_ID))) {
        LOG("[multiplayer] invalid source or game build identity\n");
        supported = false;
    }
    if (supported) LOG("[multiplayer] source ID: %s, game build: %s\n", SWMP_RUNTIME_ID, game->modules[0].build_id);
    pthread_mutex_lock(&view_lock);
    view = (SwmpM1View){
        .enabled = settings_multiplayer_enabled(), .supported = supported,
    };
    pthread_mutex_unlock(&view_lock);
}

static bool start_transport(void) {
    if (rtc) return true;
    direct = swmp_direct_create();
    if (!direct) return false;
    rtcPreload();
    static const char *ice_servers[1];
    const char *stun = getenv("SAVAGEWIND_MP_STUN");
    ice_servers[0] = stun ? stun : "stun:stun.l.google.com:19302";
    SwmpWebrtcConfig config = {
        .ice_servers = ice_servers, .ice_server_count = *ice_servers[0] ? 1 : 0,
        .connect_timeout_ms = SWMP_DIRECT_TIMEOUT_MS,
    };
    rtc = swmp_webrtc_create(config, swmp_direct_signaling(direct));
    if (!rtc) {
        rtcCleanup();
        swmp_direct_destroy(direct);
        direct = NULL;
        return false;
    }
    swmp_lobby_init(&lobby, swmp_webrtc_transport(rtc));
    return true;
}

static void stop_transport(void) {
    if (!rtc) return;
    swmp_webrtc_destroy(rtc);
    swmp_direct_destroy(direct);
    rtc = NULL;
    direct = NULL;
    reply_connection = 0;
    rtcCleanup();
}

void swmp_m1_shutdown(void) {
    stop_transport();
    memset(&lobby, 0, sizeof lobby);
    lobby_generation = 0;
    clipboard_error = false;
    pthread_mutex_lock(&view_lock);
    view = (SwmpM1View){0};
    roster = (SwmpM1Roster){0};
    local_position = (SwmpPosition){0};
    command = (Command){0};
    stop_requested = false;
    pthread_mutex_unlock(&view_lock);
}

RT_ON_RESET(multiplayer_reset) { swmp_m1_shutdown(); }

void swmp_m1_set_enabled(bool enabled) {
    pthread_mutex_lock(&view_lock);
    settings_multiplayer_set(enabled);
    view.enabled = enabled;
    if (!enabled) {
        // Hide admitted peers immediately. The window thread closes the session before processing another action,
        // even if a menu turns multiplayer back on before the next poll.
        view = (SwmpM1View){.supported = view.supported};
        roster = (SwmpM1Roster){0};
        local_position = (SwmpPosition){0};
        command = (Command){0};
        stop_requested = true;
    }
    pthread_mutex_unlock(&view_lock);
}

SwmpM1View swmp_m1_view(void) {
    pthread_mutex_lock(&view_lock);
    SwmpM1View snapshot = view;
    pthread_mutex_unlock(&view_lock);
    return snapshot;
}

SwmpM1Roster swmp_m1_roster(void) {
    pthread_mutex_lock(&view_lock);
    SwmpM1Roster snapshot = roster;
    pthread_mutex_unlock(&view_lock);
    return snapshot;
}

void swmp_m1_publish_position(uint64_t generation, const SwmpPlayerSnapshot *position, uint64_t now_ms) {
    pthread_mutex_lock(&view_lock);
    if (generation && generation == roster.lobby_generation)
        local_position = (SwmpPosition){.snapshot = *position, .received_at = now_ms};
    pthread_mutex_unlock(&view_lock);
}

bool swmp_m1_can(const SwmpM1View *v, SwmpM1Action action) {
    if (!v->enabled || !v->supported) return false;
    bool offline = v->session.state == SWMP_SESSION_OFFLINE && !v->joining;
    if (action == SWMP_M1_LEAVE) return !offline || v->pending;
    if (v->pending) return false;
    bool host = v->session.role == SWMP_ROLE_HOST;
    switch (action) {
    case SWMP_M1_HOST:
    case SWMP_M1_JOIN: return offline;
    case SWMP_M1_START: return host && v->session.state == SWMP_SESSION_LOBBY;
    case SWMP_M1_NEW_INVITE: return host && v->direct != SWMP_DIRECT_PREPARING &&
                                   swmp_session_peer_count(&v->session) < SWMP_MAX_PEERS;
    case SWMP_M1_COPY: return v->direct == SWMP_DIRECT_INVITE ||
                              (v->joining && v->direct == SWMP_DIRECT_REPLY);
    case SWMP_M1_PASTE_REPLY: return host && v->direct == SWMP_DIRECT_INVITE;
    default: return false;
    }
}

void swmp_m1_submit(SwmpM1Action action, const char *name) {
    pthread_mutex_lock(&view_lock);
    if (swmp_m1_can(&view, action)) {
        view.pending = true;
        command.action = action;
        snprintf(command.name, sizeof command.name, "%s", name ? name : "");
    }
    pthread_mutex_unlock(&view_lock);
}

void swmp_m1_update(void) {
    pthread_mutex_lock(&view_lock);
    bool enabled = view.enabled && view.supported;
    bool stop = stop_requested;
    stop_requested = false;
    Command pending = command;
    SwmpPosition sample = local_position;
    uint64_t sample_lobby = roster.lobby_generation;
    command = (Command){0};
    pthread_mutex_unlock(&view_lock);

    if (stop) {
        stop_transport();
        memset(&lobby, 0, sizeof lobby);
        clipboard_error = false;
    }
    if (!enabled) return;

    uint64_t now = plat_now_ns() / UINT64_C(1000000);
    if (direct) swmp_direct_update(direct, now);
    if (pending.action) clipboard_error = false;

    bool new_lobby = false;
    if (pending.action == SWMP_M1_HOST || pending.action == SWMP_M1_JOIN) {
        if (start_transport()) {
            if (pending.action == SWMP_M1_HOST) {
                uint64_t session_id;
                if (!swmp_random_bytes(&session_id, sizeof session_id) || !session_id)
                    snprintf(lobby.error, sizeof lobby.error, "Could not generate a session ID");
                else
                    new_lobby = swmp_lobby_host(&lobby, &compatibility, pending.name, NULL, session_id);
            } else {
                char *invite = SDL_GetClipboardText();
                new_lobby = swmp_lobby_join(&lobby, &compatibility, pending.name, NULL, invite, now);
                clipboard_error = !new_lobby;
                SDL_free(invite);
            }
        } else {
            snprintf(lobby.error, sizeof lobby.error, "Direct multiplayer is unavailable");
        }
    } else if (pending.action == SWMP_M1_LEAVE) {
        reply_connection = 0;
        if (rtc) swmp_lobby_leave(&lobby);
        else memset(&lobby, 0, sizeof lobby);
    } else if (pending.action == SWMP_M1_START && rtc) {
        swmp_lobby_start(&lobby);
    } else if (pending.action == SWMP_M1_NEW_INVITE) {
        lobby.error[0] = 0;
        reply_connection = 0;
        swmp_direct_new_invite(direct);
    } else if (pending.action == SWMP_M1_COPY) {
        const char *message = swmp_direct_message(direct);
        clipboard_error = !message || !SDL_SetClipboardText(message);
    } else if (pending.action == SWMP_M1_PASTE_REPLY) {
        char *reply = SDL_GetClipboardText();
        reply_connection = swmp_direct_accept_reply(direct, reply);
        clipboard_error = !reply_connection;
        if (reply_connection) LOG("[multiplayer] reply applied, waiting for peer\n");
        SDL_free(reply);
    }
    if (new_lobby && !++lobby_generation) ++lobby_generation;
    if (rtc) {
        swmp_webrtc_expire(rtc, now);
        bool waiting_welcome = lobby.waiting_welcome;
        swmp_lobby_poll(&lobby, now);
        SwmpPlayerSnapshot position = {0};
        if (sample_lobby == lobby_generation && swmp_position_fresh(&sample, now))
            position = sample.snapshot;
        swmp_lobby_send_position(&lobby, &position, now);
        if (lobby.joining && lobby.waiting_welcome && !waiting_welcome)
            LOG("[multiplayer] lobby handshake sent, waiting for host\n");
        if (reply_connection && !swmp_webrtc_has_link(rtc, reply_connection)) {
            snprintf(lobby.error, sizeof lobby.error, "Peer connection ended before lobby admission");
            reply_connection = 0;
        }
        for (size_t i = 0; reply_connection && i < SWMP_LOBBY_LINKS; i++) {
            const SwmpLobbyLink *link = &lobby.links[i];
            if (link->connection == reply_connection && link->peer != SWMP_UNASSIGNED_PEER)
                reply_connection = 0;
        }
    }

    SwmpM1Roster admitted = {0};
    if (lobby.session.state == SWMP_SESSION_LOBBY || lobby.session.state == SWMP_SESSION_PLAYING) {
        admitted.lobby_generation = lobby_generation;
        admitted.remotes = swmp_session_remote_roster(&lobby.session);
        memcpy(admitted.positions, lobby.positions, sizeof admitted.positions);
    }
    pthread_mutex_lock(&view_lock);
    if (stop_requested) {
        pthread_mutex_unlock(&view_lock);
        return;
    }
    bool failed = lobby.error[0] != 0;
    bool new_error = failed && (!view.failed || pending.action);
    size_t before = swmp_session_peer_count(&view.session);
    size_t after = swmp_session_peer_count(&lobby.session);
    view.pending = command.action != 0;
    view.joining = lobby.joining;
    view.awaiting_peer = reply_connection != 0;
    view.session = lobby.session;
    view.rejection = lobby.rejection;
    view.failed = failed;
    view.direct = lobby.joining && lobby.waiting_welcome ? SWMP_DIRECT_IDLE : swmp_direct_state(direct);
    view.clipboard_error = clipboard_error;
    if (roster.lobby_generation != admitted.lobby_generation)
        local_position = (SwmpPosition){0};
    roster = admitted;
    pthread_mutex_unlock(&view_lock);
    if (before != after) LOG("[multiplayer] lobby peers: %zu\n", after);
    if (new_error) {
        // Rejection text crosses the wire. Use the reason code for logging and UI in that case.
        if (lobby.rejection) LOG("[multiplayer] join rejected (%d)\n", lobby.rejection);
        else LOG("[multiplayer] %s\n", lobby.error);
    }
    if (lobby.session.state == SWMP_SESSION_OFFLINE && !lobby.joining) stop_transport();
}
#endif
