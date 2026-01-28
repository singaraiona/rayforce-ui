// include/rfui/grid_renderer.h
#ifndef RFUI_GRID_RENDERER_H
#define RFUI_GRID_RENDERER_H

#include "widget.h"

#ifdef __cplusplus
extern "C" {
#endif

// Render a grid widget
// widget->render_data should be a Rayforce table (keyed list)
nil_t rfui_render_grid(rfui_widget_t* widget);

#ifdef __cplusplus
}
#endif

#endif // RFUI_GRID_RENDERER_H
