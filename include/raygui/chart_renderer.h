// include/raygui/chart_renderer.h
#ifndef RAYGUI_CHART_RENDERER_H
#define RAYGUI_CHART_RENDERER_H

#include "widget.h"

#ifdef __cplusplus
extern "C" {
#endif

// Render a chart widget
// widget->render_data should be a table with numeric columns
nil_t raygui_render_chart(raygui_widget_t* widget);

#ifdef __cplusplus
}
#endif

#endif // RAYGUI_CHART_RENDERER_H
