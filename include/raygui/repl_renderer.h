// include/raygui/repl_renderer.h
#ifndef RAYGUI_REPL_RENDERER_H
#define RAYGUI_REPL_RENDERER_H

#include "widget.h"

#ifdef __cplusplus
extern "C" {
#endif

// Render the REPL widget
nil_t raygui_render_repl(raygui_widget_t* widget);

// Add result text to REPL output
nil_t raygui_repl_add_result(raygui_widget_t* widget, const char* text);

// Free REPL-specific ui_state (must be called before widget_destroy for REPL widgets)
nil_t raygui_repl_free_state(raygui_widget_t* widget);

#ifdef __cplusplus
}
#endif

#endif // RAYGUI_REPL_RENDERER_H
