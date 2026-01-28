// include/raygui/widget_registry.h
#ifndef RAYGUI_WIDGET_REGISTRY_H
#define RAYGUI_WIDGET_REGISTRY_H

#include "widget.h"

#ifdef __cplusplus
extern "C" {
#endif

// Initialize widget registry
nil_t raygui_registry_init(nil_t);

// Destroy widget registry
nil_t raygui_registry_destroy(nil_t);

// Add widget to registry (takes ownership of widget pointer)
nil_t raygui_registry_add(raygui_widget_t* widget);

// Render all widgets - called from UI main loop
nil_t raygui_registry_render(nil_t);

// Update widget data - called when MSG_DRAW received
// Returns old render_data that should be queued for drop
obj_p raygui_registry_update_data(raygui_widget_t* widget, obj_p new_data);

#ifdef __cplusplus
}
#endif

#endif // RAYGUI_WIDGET_REGISTRY_H
