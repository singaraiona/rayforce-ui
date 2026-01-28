// include/raygui/grid_renderer.h
#ifndef RAYGUI_GRID_RENDERER_H
#define RAYGUI_GRID_RENDERER_H

#include "widget.h"

#ifdef __cplusplus
extern "C" {
#endif

// Render a grid widget
// widget->render_data should be a Rayforce table (keyed list)
nil_t raygui_render_grid(raygui_widget_t* widget);

#ifdef __cplusplus
}
#endif

#endif // RAYGUI_GRID_RENDERER_H
