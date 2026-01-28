// include/raygui/ui.h
#ifndef RAYGUI_UI_H
#define RAYGUI_UI_H

#include "../../deps/rayforce/core/rayforce.h"

#ifdef __cplusplus
extern "C" {
#endif

// Initialize GLFW and ImGui
// Returns 0 on success, -1 on failure
i32_t raygui_ui_init(nil_t);

// Run the main UI loop
// Returns exit code
i32_t raygui_ui_run(nil_t);

// Cleanup GLFW and ImGui
nil_t raygui_ui_destroy(nil_t);

// Check if UI should continue running
b8_t raygui_ui_should_run(nil_t);

// Wake UI from another thread (called after pushing to ray_to_ui queue)
nil_t raygui_ui_wake(nil_t);

#ifdef __cplusplus
}
#endif

#endif // RAYGUI_UI_H
