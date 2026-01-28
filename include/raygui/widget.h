// include/raygui/widget.h
#ifndef RAYGUI_WIDGET_H
#define RAYGUI_WIDGET_H

#include "../../deps/rayforce/core/rayforce.h"

typedef enum raygui_widget_type_t {
    RAYGUI_WIDGET_GRID,
    RAYGUI_WIDGET_CHART,
    RAYGUI_WIDGET_TEXT,
    RAYGUI_WIDGET_REPL
} raygui_widget_type_t;

typedef struct raygui_widget_t {
    raygui_widget_type_t type;
    char* name;
    obj_p data;           // Base data from draw()
    obj_p post_query;     // Expression applied before render
    obj_p on_select;      // Callback function

    // UI state (UI thread only)
    b8_t is_open;
    u32_t dock_id;
    raw_p ui_state;       // Type-specific UI state
    obj_p render_data;    // Current data for rendering
} raygui_widget_t;

// Create widget struct (called from Rayforce thread)
raygui_widget_t* raygui_widget_create(raygui_widget_type_t type, const char* name);

// Destroy widget struct
nil_t raygui_widget_destroy(raygui_widget_t* w);

// Format widget for display
char* raygui_widget_format(raygui_widget_t* w);

// Get type name
const char* raygui_widget_type_name(raygui_widget_type_t type);

#endif // RAYGUI_WIDGET_H
