#ifdef SWMP_M1_TEST
#include "../multiplayer.h"

#include "i18n.h"
#include "imgui.h"
#include "osk.h"

static char invite[SWMP_INVITE_CODE_SIZE];

void multiplayer_tab_reset() { invite[0] = 0; }

void multiplayer_tab_draw() {
    SwmpM1View view = swmp_m1_view();
    if (!view.configured) {
        ImGui::TextWrapped("%s", tr("Set an HTTPS rendezvous server to use multiplayer."));
        return;
    }
    if (view.failed) {
        switch (view.rejection) {
        case SWMP_REJECT_PROTOCOL: ImGui::TextUnformatted(tr("Incompatible multiplayer protocol")); break;
        case SWMP_REJECT_BUILD: ImGui::TextUnformatted(tr("Incompatible game, Savage Wind, or mod build")); break;
        case SWMP_REJECT_FEATURES: ImGui::TextUnformatted(tr("Required multiplayer features are unavailable")); break;
        case SWMP_REJECT_FULL: ImGui::TextUnformatted(tr("Lobby is full")); break;
        default: ImGui::TextUnformatted(tr("Multiplayer connection ended")); break;
        }
    }
    if (!view.joining && view.session.state == SWMP_SESSION_OFFLINE) {
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
    if (view.joining || (view.session.role == SWMP_ROLE_HOST && !view.invite[0])) {
        ImGui::TextUnformatted(tr("Connecting to multiplayer"));
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
