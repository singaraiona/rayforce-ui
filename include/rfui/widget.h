// include/rfui/widget.h
#ifndef RFUI_WIDGET_H
#define RFUI_WIDGET_H

#include "../../deps/rayforce/core/rayforce.h"

typedef enum rfui_widget_type_t {
    RFUI_WIDGET_GRID,
    RFUI_WIDGET_CHART,
    RFUI_WIDGET_TEXT,
    RFUI_WIDGET_REPL
} rfui_widget_type_t;

typedef struct rfui_widget_t {
    rfui_widget_type_t type;
    char* name;
    obj_p data;           // Base data from draw()
    obj_p post_query;     // Expression applied before render
    obj_p on_select;      // Callback function

    // UI state (UI thread only)
    b8_t is_open;
    u32_t dock_id;
    raw_p ui_state;       // Type-specific UI state
    obj_p render_data;    // Current data for rendering
} rfui_widget_t;

// Create widget struct (called from Rayforce thread)
rfui_widget_t* rfui_widget_create(rfui_widget_type_t type, const char* name);

// Destroy widget struct
nil_t rfui_widget_destroy(rfui_widget_t* w);

// Format widget for display
char* rfui_widget_format(rfui_widget_t* w);

// Get type name
const char* rfui_widget_type_name(rfui_widget_type_t type);

#endif // RFUI_WIDGET_H
