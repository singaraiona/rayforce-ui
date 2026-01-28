// src/text_renderer.cpp
// Text widget renderer for displaying formatted Rayforce object output
//
// NOTE: All Rayforce obj_p formatting is done on the Rayforce thread
// (in fn_draw) before sending to the UI. The text renderer only displays
// the pre-formatted string stored in widget->ui_state. This avoids calling
// Rayforce runtime functions (obj_fmt, drop_obj) from the UI thread, which
// has no Rayforce runtime context (__VM is NULL on the UI thread).

#include <stdio.h>
#include <string.h>

#include "imgui.h"

// Make rayforce headers C++ compatible by redefining _Static_assert
#define _Static_assert static_assert

extern "C" {
#include "../include/rfui/text_renderer.h"
#include "../include/rfui/widget.h"
}

extern "C" {

nil_t rfui_render_text(rfui_widget_t* widget) {
    if (widget == nullptr) {
        return;
    }

    // ui_state holds pre-formatted text string (set by UI DRAW handler)
    const char* text = (const char*)widget->ui_state;

    if (text == nullptr) {
        ImGui::TextDisabled("No data");
        return;
    }

    // Use large font for label display
    ImGuiIO& io = ImGui::GetIO();
    if (io.Fonts->Fonts.Size > 1) {
        ImGui::PushFont(io.Fonts->Fonts[1]);
    }

    // Center text vertically and horizontally in available space
    ImVec2 avail = ImGui::GetContentRegionAvail();
    ImVec2 text_size = ImGui::CalcTextSize(text);
    ImVec2 cursor = ImGui::GetCursorPos();
    if (text_size.x < avail.x)
        ImGui::SetCursorPosX(cursor.x + (avail.x - text_size.x) * 0.5f);
    if (text_size.y < avail.y)
        ImGui::SetCursorPosY(cursor.y + (avail.y - text_size.y) * 0.5f);

    ImGui::TextUnformatted(text);

    if (io.Fonts->Fonts.Size > 1) {
        ImGui::PopFont();
    }
}

} // extern "C"
