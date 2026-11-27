#ifdef SW_MULTIPLAYER
#include "multiplayer.h"

#include "i18n.h"
#include "imgui.h"
#include "osk.h"
#include <algorithm>
#include <cfloat>
extern "C" {
#include "../settings.h"
}

const char *multiplayer_status(const SwmpM1View &view) {
    if (!view.enabled) return tr("Off");
    if (!view.supported) return tr("Multiplayer is unavailable for this game build.");
    if (view.clipboard_error) return tr("Check the invite or reply on your clipboard.");
    if (view.failed && !view.pending) {
        switch (view.rejection) {
        case SWMP_REJECT_PROTOCOL: return tr("Incompatible multiplayer protocol");
        case SWMP_REJECT_BUILD: return tr("Incompatible game, Savage Wind, or mod build");
        case SWMP_REJECT_FEATURES: return tr("Required multiplayer features are unavailable");
        case SWMP_REJECT_FULL: return tr("Lobby is full");
        default: return tr("Multiplayer connection ended");
        }
    }
    if (view.direct == SWMP_DIRECT_INVITE) return tr("Send the invite and paste the reply.");
    if (view.joining && view.direct == SWMP_DIRECT_REPLY) return tr("Send the reply to the host.");
    if (view.direct == SWMP_DIRECT_EXPIRED) return tr("Create a new invite.");
    if (view.pending || view.joining || view.awaiting_peer || view.direct == SWMP_DIRECT_PREPARING)
        return tr("Connecting to multiplayer");
    if (swmp_session_peer_count(&view.session) > 1) return tr("Connected");
    return view.session.role == SWMP_ROLE_HOST ? tr("Waiting for players") : tr("Offline");
}

void multiplayer_tab_draw() {
    SwmpM1View view = swmp_m1_view();
    if (ImGui::Checkbox(tr("Enable multiplayer"), &view.enabled)) {
        swmp_m1_set_enabled(view.enabled);
        settings_save();
        view = swmp_m1_view();
    }
    if (!view.enabled) return;
    ImGui::TextWrapped("%s", multiplayer_status(view));
    if (!view.supported) return;
    bool sync_environment = settings_multiplayer_sync_environment();
    if (ImGui::Checkbox(tr("Sync host time and weather"), &sync_environment)) {
        settings_multiplayer_set_sync_environment(sync_environment);
        settings_save();
    }
    bool show_names = settings_multiplayer_show_names();
    if (ImGui::Checkbox(tr("Show overhead player names"), &show_names)) {
        settings_multiplayer_set_show_names(show_names);
        settings_save();
    }
    if (!view.pending && !view.joining && view.session.state == SWMP_SESSION_OFFLINE) {
        static char name[SWMP_PLAYER_NAME_SIZE] = "Link";  // i18n-skip (default player name)
        ui_input_text(tr("Player name"), name, sizeof name);
        if (ImGui::Button(tr("Host lobby"))) swmp_m1_submit(SWMP_M1_HOST, name);
        ImGui::SameLine();
        if (ImGui::Button(tr("Paste invite and join"))) swmp_m1_submit(SWMP_M1_JOIN, name);
        return;
    }
    if (swmp_m1_can(&view, SWMP_M1_COPY)) {
        const char *label = view.joining ? tr("Copy reply") : tr("Copy invite code");
        if (ImGui::Button(label)) swmp_m1_submit(SWMP_M1_COPY, nullptr);
    }
    if (swmp_m1_can(&view, SWMP_M1_PASTE_REPLY)) {
        ImGui::SameLine();
        if (ImGui::Button(tr("Paste reply"))) swmp_m1_submit(SWMP_M1_PASTE_REPLY, nullptr);
    }
    if (swmp_m1_can(&view, SWMP_M1_NEW_INVITE)) {
        if (ImGui::Button(tr("New invite"))) swmp_m1_submit(SWMP_M1_NEW_INVITE, nullptr);
    }
    for (unsigned i = 0; i < SWMP_MAX_PEERS; i++) {
        if (view.session.peers[i].state != SWMP_PEER_READY) continue;
        const char *name = view.session.peers[i].name;
        if (view.health_known & (1u << i))
            ImGui::BulletText("%s | %s %g", name, tr("Hearts"), view.health_q4[i] / 4.0);
        else
            ImGui::BulletText("%s", name);
    }
    if (swmp_m1_can(&view, SWMP_M1_START)) {
        if (ImGui::Button(tr("Start lobby"))) swmp_m1_submit(SWMP_M1_START, nullptr);
        ImGui::SameLine();
    }
    const char *leave = view.joining || view.session.state == SWMP_SESSION_OFFLINE ? tr("Cancel") : tr("Leave lobby");
    if (ImGui::Button(leave)) swmp_m1_submit(SWMP_M1_LEAVE, nullptr);
}

void multiplayer_indicators_draw(const SwmpIndicatorOverlay &overlay, float scale) {
    static const ImU32 colors[SWMP_MAX_PEERS] = {
        IM_COL32(84, 226, 242, 255), IM_COL32(255, 198, 81, 255), IM_COL32(170, 223, 106, 255),
        IM_COL32(255, 145, 165, 255), IM_COL32(173, 166, 255, 255), IM_COL32(113, 187, 255, 255),
        IM_COL32(255, 159, 105, 255), IM_COL32(100, 230, 177, 255),
    };
    ImDrawList *draw = ImGui::GetBackgroundDrawList();
    draw->PushClipRect(ImVec2(overlay.clip[0], overlay.clip[1]), ImVec2(overlay.clip[2], overlay.clip[3]), true);
    ImFont *font = ImGui::GetFont();
    const float size = std::max(13.0f, 21 * scale);
    for (unsigned i = 0; i < SWMP_MAX_PEERS; i++) {
        if (!(overlay.visible & (1u << i))) continue;
        const ImVec2 point(overlay.points[i][0], overlay.points[i][1]);
        const char *name = overlay.names[i];
        const ImVec2 text = font->CalcTextSizeA(size, FLT_MAX, 0, name);
        const float radius = std::max(4.0f, 7 * scale), pad = 4 * scale;
        if (overlay.map) {
            draw->AddCircleFilled(point, radius + 1.5f, IM_COL32(15, 22, 28, 255));
            draw->AddCircleFilled(point, radius, colors[i]);
        }
        const ImVec2 label(overlay.map ? point.x + radius + 2 * pad : point.x - text.x * 0.5f,
                           overlay.map ? point.y - text.y * 0.5f : point.y - text.y);
        draw->AddRectFilled(ImVec2(label.x - pad, label.y - pad * 0.5f),
                            ImVec2(label.x + text.x + pad, label.y + text.y + pad * 0.5f),
                            IM_COL32(15, 22, 28, 220), pad);
        draw->AddText(font, size, label, IM_COL32_WHITE, name);
    }
    draw->PopClipRect();
}

void multiplayer_notices_draw(float scale) {
    SwmpNotice notices[SWMP_MAX_PEERS];
    size_t count = swmp_m1_notices(notices);
    if (!count) return;
    const ImVec2 size = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(size.x - 20 * scale, 20 * scale), ImGuiCond_Always, ImVec2(1, 0));
    ImGui::SetNextWindowBgAlpha(0.85f);
    ImGui::Begin("##multiplayer_notifications", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs |
                 ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoSavedSettings);
    for (size_t i = 0; i < count; i++) {
        const SwmpNotice &notice = notices[i];
        if (notice.type == SWMP_NOTICE_LOCAL_LEFT)
            ImGui::TextUnformatted(tr("You disconnected from multiplayer"));
        else if (notice.type == SWMP_NOTICE_LEFT)
            ImGui::Text(tr("[%s] disconnected from the game"), notice.name);
        else
            ImGui::Text(tr("[%s] connected to the game"), notice.name);
    }
    ImGui::End();
}
#endif
