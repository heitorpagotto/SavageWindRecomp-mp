#ifdef SWMP_M1_TEST
#include "multiplayer.h"

#include "hle.h"
#include "package_marker.h"
#include "rendezvous.h"

#include <curl/curl.h>
#include <rtc/rtc.h>
#include <string.h>

typedef struct {
    SwmpM1Action action;
    char name[SWMP_PLAYER_NAME_SIZE];
    char invite[SWMP_INVITE_CODE_SIZE];
} Command;

// The window thread publishes copies to the render and game threads.
static pthread_mutex_t view_lock = PTHREAD_MUTEX_INITIALIZER;
static SwmpM1View view;
static SwmpM1Roster roster;
static Command command;

// Window thread only.
static bool enabled;
static const char *origin;
static SwmpCompatibility compatibility;
static SwmpRendezvous *signaling;
static SwmpWebrtc *rtc;
static SwmpLobby lobby;
static uint64_t lobby_generation;

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
    if (!game || !game->version || strcmp(game->version, "1.9.0") || !*game->nmodules ||
        !swmp_package_marker_enabled(getenv("SAVAGEWIND_ROMFS_OVERLAY"))) return;
    compatibility = (SwmpCompatibility){
        .protocol = SWMP_PROTOCOL_VERSION, .mod_version = {0, 1, 0}, .game_version = {1, 9, 0},
    };
    // A gamelib ID also encodes local compiler/CPU flags. Use the common source identity for wire compatibility.
    if (!read_build_id(compatibility.game_build, sizeof compatibility.game_build, game->modules[0].build_id) ||
        !read_build_id(compatibility.savage_build, sizeof compatibility.savage_build, SWMP_RUNTIME_ID)) {
        LOG("[multiplayer] invalid source or game build identity\n");
        return;
    }
    origin = getenv("SAVAGEWIND_MP_RENDEZVOUS");
    enabled = true;
    pthread_mutex_lock(&view_lock);
    view = (SwmpM1View){.enabled = true, .configured = origin && *origin};
    pthread_mutex_unlock(&view_lock);
    LOG("[multiplayer] M1 development marker validated\n");
}

static bool start_transport(void) {
    if (rtc) return true;
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) return false;
    signaling = swmp_rendezvous_create(getenv("SAVAGEWIND_MP_CA_BUNDLE"));
    if (signaling) {
        rtcPreload();
        static const char *ice_servers[2];
        const char *stun = getenv("SAVAGEWIND_MP_STUN");
        const char *turn = getenv("SAVAGEWIND_MP_TURN");
        const char *relay = getenv("SAVAGEWIND_MP_RELAY_ONLY");
        int count = 0;
        if (stun && *stun) ice_servers[count++] = stun;
        if (turn && *turn) ice_servers[count++] = turn;
        SwmpWebrtcConfig config = {
            .ice_servers = ice_servers, .ice_server_count = count,
            .relay_only = relay && !strcmp(relay, "1"),
        };
        rtc = swmp_webrtc_create(config, swmp_rendezvous_signaling(signaling));
        if (rtc) {
            swmp_lobby_init(&lobby, swmp_webrtc_transport(rtc));
            return true;
        }
        rtcCleanup();
        swmp_rendezvous_destroy(signaling);
        signaling = NULL;
    }
    curl_global_cleanup();
    return false;
}

static void stop_transport(void) {
    if (!rtc) return;
    swmp_webrtc_destroy(rtc);
    swmp_rendezvous_destroy(signaling);
    rtc = NULL;
    signaling = NULL;
    rtcCleanup();
    curl_global_cleanup();
}

void swmp_m1_shutdown(void) {
    enabled = false;
    origin = NULL;
    stop_transport();
    memset(&lobby, 0, sizeof lobby);
    lobby_generation = 0;
    pthread_mutex_lock(&view_lock);
    view = (SwmpM1View){0};
    roster = (SwmpM1Roster){0};
    command = (Command){0};
    pthread_mutex_unlock(&view_lock);
}

RT_ON_RESET(multiplayer_reset) { swmp_m1_shutdown(); }

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

void swmp_m1_submit(SwmpM1Action action, const char *name, const char *invite) {
    pthread_mutex_lock(&view_lock);
    if (view.enabled && !command.action) {
        command.action = action;
        snprintf(command.name, sizeof command.name, "%s", name ? name : "");
        snprintf(command.invite, sizeof command.invite, "%s", invite ? invite : "");
    }
    pthread_mutex_unlock(&view_lock);
}

void swmp_m1_update(void) {
    if (!enabled) return;

    pthread_mutex_lock(&view_lock);
    Command pending = command;
    command = (Command){0};
    pthread_mutex_unlock(&view_lock);
    uint64_t now = plat_now_ns() / UINT64_C(1000000);

    bool new_lobby = false;
    if (pending.action == SWMP_M1_HOST || pending.action == SWMP_M1_JOIN) {
        if (origin && *origin && start_transport()) {
            if (pending.action == SWMP_M1_HOST) {
                uint64_t session_id;
                if (!swmp_random_bytes(&session_id, sizeof session_id) || !session_id)
                    snprintf(lobby.error, sizeof lobby.error, "Could not generate a session ID");
                else
                    new_lobby = swmp_lobby_host(&lobby, &compatibility, pending.name, origin, session_id);
            } else {
                new_lobby = swmp_lobby_join(&lobby, &compatibility, pending.name, origin, pending.invite, now);
            }
        } else {
            snprintf(lobby.error, sizeof lobby.error, "Lobby signaling is unavailable");
        }
    } else if (pending.action == SWMP_M1_LEAVE && rtc) {
        swmp_lobby_leave(&lobby);
    } else if (pending.action == SWMP_M1_START && rtc) {
        swmp_lobby_start(&lobby);
    }
    if (new_lobby && !++lobby_generation) ++lobby_generation;
    if (rtc) {
        swmp_rendezvous_update(signaling, now);
        swmp_lobby_poll(&lobby, now);
    }

    SwmpM1View snapshot = {
        .enabled = true, .configured = origin && *origin, .joining = lobby.joining,
        .session = lobby.session, .rejection = lobby.rejection, .failed = lobby.error[0] != 0,
    };
    SwmpM1Roster admitted = {0};
    if (lobby.session.state == SWMP_SESSION_LOBBY || lobby.session.state == SWMP_SESSION_PLAYING) {
        admitted.lobby_generation = lobby_generation;
        admitted.remotes = swmp_session_remote_roster(&lobby.session);
        admitted.local_peer = lobby.session.local_peer;
    }
    memcpy(snapshot.invite, lobby.invite, sizeof snapshot.invite);
    pthread_mutex_lock(&view_lock);
    bool new_error = snapshot.failed && (!view.failed || pending.action);
    size_t before = swmp_session_peer_count(&view.session);
    size_t after = swmp_session_peer_count(&snapshot.session);
    view = snapshot;
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
