// src/text_renderer.cpp
// Text widget renderer for displaying formatted Rayforce object output

#include <stdio.h>
#include <string.h>

#include "imgui.h"

// Make rayforce headers C++ compatible by redefining _Static_assert
#define _Static_assert static_assert

extern "C" {
#include "../include/raygui/text_renderer.h"
#include "../include/raygui/widget.h"
#include "../deps/rayforce/core/format.h"
}

extern "C" {

nil_t raygui_render_text(raygui_widget_t* widget) {
    if (widget == nullptr) {
        return;
    }

    obj_p data = widget->render_data;

    // Check if we have data
    if (data == nullptr) {
        ImGui::TextDisabled("No data");
        return;
    }

    // Format the object to a string representation
    // B8_TRUE = full format with limits (readable multiline output)
    obj_p fmt = obj_fmt(data, B8_TRUE);

    if (fmt == nullptr || fmt->type != TYPE_C8) {
        ImGui::TextDisabled("Failed to format data");
        if (fmt != nullptr) {
            drop_obj(fmt);
        }
        return;
    }

    // Display in a scrollable child region for large output
    // ImGuiWindowFlags_HorizontalScrollbar for wide content
    ImGui::BeginChild("##textcontent", ImVec2(0, 0), false,
                      ImGuiWindowFlags_HorizontalScrollbar);

    // Use TextUnformatted for efficiency with potentially large strings
    // AS_C8(fmt) returns the char* data, fmt->len is the length
    ImGui::TextUnformatted(AS_C8(fmt), AS_C8(fmt) + fmt->len);

    ImGui::EndChild();

    // Clean up formatted string
    drop_obj(fmt);
}

} // extern "C"
