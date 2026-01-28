// include/raygui/message.h
#ifndef RAYGUI_MESSAGE_H
#define RAYGUI_MESSAGE_H

#include "../../deps/rayforce/core/rayforce.h"

// Forward declaration
struct raygui_widget_t;

// UI → Rayforce message types
typedef enum raygui_ui_msg_type_t {
    RAYGUI_MSG_EVAL,           // Evaluate expression
    RAYGUI_MSG_SET_POST_QUERY, // Set widget post_query
    RAYGUI_MSG_DROP,           // Drop obj_p after render
    RAYGUI_MSG_QUIT            // Shutdown
} raygui_ui_msg_type_t;

// Rayforce → UI message types
typedef enum raygui_ray_msg_type_t {
    RAYGUI_MSG_WIDGET_CREATED, // New widget panel
    RAYGUI_MSG_DRAW,           // Widget data update
    RAYGUI_MSG_RESULT          // REPL result
} raygui_ray_msg_type_t;

// UI → Rayforce message
typedef struct raygui_ui_msg_t {
    raygui_ui_msg_type_t type;
    char* expr;                      // Expression string (owned, must free)
    obj_p obj;                       // Object to drop
    struct raygui_widget_t* widget;  // Target widget
} raygui_ui_msg_t;

// Rayforce → UI message
typedef struct raygui_ray_msg_t {
    raygui_ray_msg_type_t type;
    struct raygui_widget_t* widget;  // Target widget
    obj_p data;                      // Data for rendering
    char* text;                      // Result text (owned, must free)
} raygui_ray_msg_t;

#endif // RAYGUI_MESSAGE_H
