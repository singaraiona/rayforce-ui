// src/widget.c
#include "../include/raygui/widget.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// strdup is POSIX, not C standard - provide explicit declaration
extern char* strdup(const char* s);

const char* raygui_widget_type_name(raygui_widget_type_t type) {
    switch (type) {
        case RAYGUI_WIDGET_GRID:  return "grid";
        case RAYGUI_WIDGET_CHART: return "chart";
        case RAYGUI_WIDGET_TEXT:  return "text";
        case RAYGUI_WIDGET_REPL:  return "repl";
        default: return "unknown";
    }
}

raygui_widget_t* raygui_widget_create(raygui_widget_type_t type, const char* name) {
    raygui_widget_t* w = malloc(sizeof(raygui_widget_t));
    if (!w) return NULL;

    w->type = type;
    w->name = strdup(name);
    w->data = NULL;
    w->post_query = NULL;
    w->on_select = NULL;
    w->is_open = B8_TRUE;
    w->dock_id = 0;
    w->ui_state = NULL;
    w->render_data = NULL;

    return w;
}

nil_t raygui_widget_destroy(raygui_widget_t* w) {
    if (!w) return;

    free(w->name);
    if (w->data) drop_obj(w->data);
    if (w->post_query) drop_obj(w->post_query);
    if (w->on_select) drop_obj(w->on_select);
    if (w->render_data) drop_obj(w->render_data);
    free(w);
}

char* raygui_widget_format(raygui_widget_t* w) {
    char* buf = malloc(256);
    snprintf(buf, 256, "widget<%s:\"%s\">",
             raygui_widget_type_name(w->type), w->name);
    return buf;
}
