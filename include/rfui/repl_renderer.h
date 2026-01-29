// include/rfui/repl_renderer.h
#ifndef RFUI_REPL_RENDERER_H
#define RFUI_REPL_RENDERER_H

#include "../../deps/rayforce/core/rayforce.h"

#ifdef __cplusplus
extern "C" {
#endif

// Initialize REPL state (call once at startup)
nil_t rfui_repl_init(nil_t);

// Render REPL content (call each frame, inside main window)
nil_t rfui_repl_render(nil_t);

// Add result text to REPL output (called when MSG_RESULT received)
nil_t rfui_repl_add_result_text(const char* text);

// Load a script file via REPL (shows in history, evaluates)
nil_t rfui_repl_load_file(const char* path);

// Destroy REPL state
nil_t rfui_repl_destroy(nil_t);

#ifdef __cplusplus
}
#endif

#endif // RFUI_REPL_RENDERER_H
