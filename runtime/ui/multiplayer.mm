#ifdef SW_MULTIPLAYER
#include "multiplayer.h"

#include "i18n.h"
#include "imgui.h"
#include "osk.h"
extern "C" {
#include "../settings.h"
}

static char invite[SWMP_INVITE_CODE_SIZE];

void multiplayer_tab_reset() { invite[0] = 0; }

const char *multiplayer_status(const SwmpM1View &view) {
    if (!view.enabled) return tr("Off");
    if (!view.supported) return tr("Multiplayer is unavailable for this game build.");
    if (!view.configured) return tr("Set an HTTPS rendezvous server to use multiplayer.");
    if (view.failed && !view.pending) {
        switch (view.rejection) {
        case SWMP_REJECT_PROTOCOL: return tr("Incompatible multiplayer protocol");
        case SWMP_REJECT_BUILD: return tr("Incompatible game, Savage Wind, or mod build");
        case SWMP_REJECT_FEATURES: return tr("Required multiplayer features are unavailable");
        case SWMP_REJECT_FULL: return tr("Lobby is full");
        default: return tr("Multiplayer connection ended");
        }
    }
    if (view.pending || view.joining || (view.session.role == SWMP_ROLE_HOST && !view.invite[0]))
        return tr("Connecting to multiplayer");
    return view.session.state == SWMP_SESSION_OFFLINE ? tr("Offline") : tr("Connected");
}

void multiplayer_tab_draw() {
    SwmpM1View view = swmp_m1_view();
    if (ImGui::Checkbox(tr("Enable multiplayer"), &view.enabled)) {
        swmp_m1_set_enabled(view.enabled);
        settings_save();
        view = swmp_m1_view();
    }
    if (!view.enabled) {
        invite[0] = 0;
        return;
    }
    if (!view.supported || !view.configured || (view.failed && !view.pending))
        ImGui::TextWrapped("%s", multiplayer_status(view));
    if (!view.supported || !view.configured) return;
    if (!view.pending && !view.joining && view.session.state == SWMP_SESSION_OFFLINE) {
        static char name[SWMP_PLAYER_NAME_SIZE] = "Link";  // i18n-skip (default player name)
        ui_input_text(tr("Player name"), name, sizeof name);
        ui_input_text(tr("Invite code"), invite, sizeof invite, ImGuiInputTextFlags_CharsUppercase);
        if (ImGui::Button(tr("Host lobby"))) swmp_m1_submit(SWMP_M1_HOST, name, nullptr);
        ImGui::SameLine();
        ImGui::BeginDisabled(!invite[0]);
        if (ImGui::Button(tr("Join lobby"))) swmp_m1_submit(SWMP_M1_JOIN, name, invite);
        ImGui::EndDisabled();
        return;
    }
    if (view.pending || view.joining || (view.session.role == SWMP_ROLE_HOST && !view.invite[0])) {
        ImGui::TextUnformatted(multiplayer_status(view));
        if (ImGui::Button(tr("Cancel"))) swmp_m1_submit(SWMP_M1_LEAVE, nullptr, nullptr);
        return;
    }
    if (view.invite[0]) {
        ImGui::Text("%s  %s", tr("Invite code"), view.invite);
        ImGui::SameLine();
        if (ImGui::Button(tr("Copy invite code"))) ImGui::SetClipboardText(view.invite);
    }
    for (unsigned i = 0; i < SWMP_MAX_PEERS; i++)
        if (view.session.peers[i].state == SWMP_PEER_READY)
            ImGui::BulletText("%s", view.session.peers[i].name);
    if (view.session.role == SWMP_ROLE_HOST && view.session.state == SWMP_SESSION_LOBBY) {
        if (ImGui::Button(tr("Start lobby"))) swmp_m1_submit(SWMP_M1_START, nullptr, nullptr);
        ImGui::SameLine();
    }
    if (ImGui::Button(tr("Leave lobby"))) swmp_m1_submit(SWMP_M1_LEAVE, nullptr, nullptr);
}
#endif
