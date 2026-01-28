// src/widget_registry.cpp
// Widget registry implementation for UI-side widget tracking

#include <vector>
#include <stdio.h>

#include "imgui.h"

// Make rayforce headers C++ compatible by redefining _Static_assert
#define _Static_assert static_assert

extern "C" {
#include "../include/raygui/widget_registry.h"
#include "../include/raygui/widget.h"
}

// Global widget storage
static std::vector<raygui_widget_t*> g_widgets;

extern "C" {

nil_t raygui_registry_init(nil_t) {
    // Nothing to do for std::vector - it's already initialized
    g_widgets.clear();
}

nil_t raygui_registry_destroy(nil_t) {
    // Free all widgets
    for (raygui_widget_t* widget : g_widgets) {
        raygui_widget_destroy(widget);
    }
    g_widgets.clear();
}

nil_t raygui_registry_add(raygui_widget_t* widget) {
    if (widget == nullptr) {
        return;
    }
    g_widgets.push_back(widget);
}

nil_t raygui_registry_render(nil_t) {
    for (raygui_widget_t* widget : g_widgets) {
        if (widget == nullptr || !widget->is_open) {
            continue;
        }

        // Begin widget window with close button
        ImGui::Begin(widget->name, (bool*)&widget->is_open);
        ImGui::End();
    }
}

obj_p raygui_registry_update_data(raygui_widget_t* widget, obj_p new_data) {
    if (widget == nullptr) {
        return nullptr;
    }

    // Store old data
    obj_p old_data = widget->render_data;

    // Set new data
    widget->render_data = new_data;

    // Return old data for caller to queue for drop
    return old_data;
}

} // extern "C"
