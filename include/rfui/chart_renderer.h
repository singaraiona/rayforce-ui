// include/rfui/chart_renderer.h
#ifndef RFUI_CHART_RENDERER_H
#define RFUI_CHART_RENDERER_H

#include "widget.h"

#ifdef __cplusplus
extern "C" {
#endif

// Render a chart widget
// widget->render_data should be a table with numeric columns
nil_t rfui_render_chart(rfui_widget_t* widget);

#ifdef __cplusplus
}
#endif

#endif // RFUI_CHART_RENDERER_H
