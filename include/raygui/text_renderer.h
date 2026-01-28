// include/raygui/text_renderer.h
#ifndef RAYGUI_TEXT_RENDERER_H
#define RAYGUI_TEXT_RENDERER_H

#include "widget.h"

#ifdef __cplusplus
extern "C" {
#endif

// Render a text widget
// widget->render_data can be any Rayforce object - displays formatted string representation
nil_t raygui_render_text(raygui_widget_t* widget);

#ifdef __cplusplus
}
#endif

#endif // RAYGUI_TEXT_RENDERER_H
