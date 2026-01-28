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
#include "../include/raygui/grid_renderer.h"
#include "../include/raygui/chart_renderer.h"
#include "../include/raygui/text_renderer.h"
#include "../include/raygui/repl_renderer.h"
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
    // NOTE: Thread safety consideration - registry_destroy is called AFTER UI loop
    // exits but BEFORE Rayforce thread is joined. This means drop_obj calls in
    // widget_destroy happen from the UI thread, not the Rayforce thread.
    // TODO: This is known technical debt. For proper thread safety, widgets with
    // render_data should queue drops to Rayforce thread before destruction.
    // Current approach is acceptable for shutdown but not ideal.
    for (raygui_widget_t* widget : g_widgets) {
        if (widget != nullptr) {
            // Free type-specific ui_state (must use delete for C++ objects)
            switch (widget->type) {
                case RAYGUI_WIDGET_REPL:
                    raygui_repl_free_state(widget);
                    break;
                default:
                    // Other widget types use plain malloc/free for ui_state
                    break;
            }
        }
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

        // Render based on widget type
        switch (widget->type) {
            case RAYGUI_WIDGET_GRID:
                raygui_render_grid(widget);
                break;
            case RAYGUI_WIDGET_CHART:
                raygui_render_chart(widget);
                break;
            case RAYGUI_WIDGET_TEXT:
                raygui_render_text(widget);
                break;
            case RAYGUI_WIDGET_REPL:
                raygui_render_repl(widget);
                break;
            default:
                ImGui::TextDisabled("Unknown widget type: %d", widget->type);
                break;
        }

        ImGui::End();
    }
}

obj_p raygui_registry_update_data(raygui_widget_t* widget, obj_p new_data) {
    if (widget == nullptr) {
        return nullptr;
    }

    // Check for same data - avoid use-after-free if caller passes same object
    if (new_data == widget->render_data) {
        return nullptr;  // Same data, no swap needed
    }

    // Store old data
    obj_p old_data = widget->render_data;

    // Set new data
    widget->render_data = new_data;

    // Return old data for caller to queue for drop
    return old_data;
}

raygui_widget_t* raygui_registry_find_by_type(raygui_widget_type_t type) {
    for (raygui_widget_t* widget : g_widgets) {
        if (widget != nullptr && widget->type == type) {
            return widget;
        }
    }
    return nullptr;
}

} // extern "C"
