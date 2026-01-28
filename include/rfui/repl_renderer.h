// include/rfui/repl_renderer.h
#ifndef RFUI_REPL_RENDERER_H
#define RFUI_REPL_RENDERER_H

#include "widget.h"

#ifdef __cplusplus
extern "C" {
#endif

// Render the REPL widget
nil_t rfui_render_repl(rfui_widget_t* widget);

// Add result text to REPL output
nil_t rfui_repl_add_result(rfui_widget_t* widget, const char* text);

// Free REPL-specific ui_state (must be called before widget_destroy for REPL widgets)
nil_t rfui_repl_free_state(rfui_widget_t* widget);

#ifdef __cplusplus
}
#endif

#endif // RFUI_REPL_RENDERER_H
