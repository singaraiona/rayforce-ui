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
#include "../include/raygui/text_renderer.h"
#include "../include/raygui/widget.h"
}

extern "C" {

nil_t raygui_render_text(raygui_widget_t* widget) {
    if (widget == nullptr) {
        return;
    }

    // ui_state holds pre-formatted text string (set by UI DRAW handler)
    const char* text = (const char*)widget->ui_state;

    if (text == nullptr) {
        ImGui::TextDisabled("No data");
        return;
    }

    // Display in a scrollable child region for large output
    ImGui::BeginChild("##textcontent", ImVec2(0, 0), false,
                      ImGuiWindowFlags_HorizontalScrollbar);

    ImGui::TextUnformatted(text);

    ImGui::EndChild();
}

} // extern "C"
