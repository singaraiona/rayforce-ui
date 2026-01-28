// include/rfui/text_renderer.h
#ifndef RFUI_TEXT_RENDERER_H
#define RFUI_TEXT_RENDERER_H

#include "widget.h"

#ifdef __cplusplus
extern "C" {
#endif

// Render a text widget
// widget->render_data can be any Rayforce object - displays formatted string representation
nil_t rfui_render_text(rfui_widget_t* widget);

#ifdef __cplusplus
}
#endif

#endif // RFUI_TEXT_RENDERER_H
