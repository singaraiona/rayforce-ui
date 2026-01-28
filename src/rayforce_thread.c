// src/rayforce_thread.c
// Include system string.h via absolute path to avoid conflict with deps/rayforce/core/string.h
#include "/usr/include/string.h"
#include <stdlib.h>
#include "../deps/rayforce/core/runtime.h"
#include "../deps/rayforce/core/poll.h"
#include "../deps/rayforce/core/symbols.h"
#include "../deps/rayforce/core/ops.h"
#include "../deps/rayforce/core/util.h"
#include "../include/raygui/context.h"
#include "../include/raygui/message.h"
#include "../include/raygui/queue.h"
#include "../include/raygui/widget.h"
#include "../include/raygui/rayforce_thread.h"
#include <GLFW/glfw3.h>

// Thread-local context for raygui functions
static __thread raygui_ctx_t* g_ctx = NULL;

// Forward declaration
static void on_ui_message(raw_p data);

// Process a single UI message
static void process_ui_message(raygui_ctx_t* ctx, raygui_ui_msg_t* msg) {
    if (!msg) return;

    switch (msg->type) {
        case RAYGUI_MSG_EVAL:
            // TODO: Evaluate expression and send result back
            // For now, just free the expression string
            if (msg->expr) {
                free(msg->expr);
            }
            break;

        case RAYGUI_MSG_SET_POST_QUERY:
            // TODO: Set widget post_query
            // For now, just free the expression string
            if (msg->expr) {
                free(msg->expr);
            }
            break;

        case RAYGUI_MSG_DROP:
            // Drop obj_p after render
            if (msg->obj) {
                drop_obj(msg->obj);
            }
            break;

        case RAYGUI_MSG_QUIT:
            // Set quit flag
            raygui_ctx_set_quit(ctx, B8_TRUE);
            // Exit the poll loop
            if (runtime_get()) {
                poll_exit(runtime_get()->poll, 0);
            }
            break;

        default:
            // Unknown message type - ignore
            break;
    }

    // Free the message struct
    free(msg);
}

// Waker callback - called when UI thread wakes the Rayforce thread
static void on_ui_message(raw_p data) {
    raygui_ctx_t* ctx = (raygui_ctx_t*)data;
    raygui_ui_msg_t* msg;

    if (!ctx) return;

    // Capture poll pointer at start to avoid use-after-free if runtime changes
    poll_p poll = runtime_get() ? runtime_get()->poll : NULL;
    (void)poll;  // Currently unused but captured for safety

    // Drain the queue and process all pending messages
    // Check quit flag to exit early on shutdown
    while (!raygui_ctx_get_quit(ctx) &&
           (msg = (raygui_ui_msg_t*)raygui_queue_pop(ctx->ui_to_ray)) != NULL) {
        process_ui_message(ctx, msg);
    }
}

// Register raygui extension types (stub for now)
static void register_raygui_types(void) {
    // Widget type uses TYPE_EXT (external) which is built-in
    // No custom type registration needed
}

// Helper: Map symbol string to widget type
static b8_t widget_type_from_symbol(i64_t sym_id, raygui_widget_type_t* out_type) {
    const char* type_str = str_from_symbol(sym_id);
    if (!type_str) return B8_FALSE;

    if (strcmp(type_str, "grid") == 0) {
        *out_type = RAYGUI_WIDGET_GRID;
        return B8_TRUE;
    } else if (strcmp(type_str, "chart") == 0) {
        *out_type = RAYGUI_WIDGET_CHART;
        return B8_TRUE;
    } else if (strcmp(type_str, "text") == 0) {
        *out_type = RAYGUI_WIDGET_TEXT;
        return B8_TRUE;
    } else if (strcmp(type_str, "repl") == 0) {
        *out_type = RAYGUI_WIDGET_REPL;
        return B8_TRUE;
    }
    return B8_FALSE;
}

// Drop function for widget external objects
// Note: We don't actually free the widget here because the UI thread owns it
// The UI will free it when the widget is closed/destroyed
static nil_t widget_drop(raw_p ptr) {
    UNUSED(ptr);
    // Widget is owned by UI thread, do not free here
}

// fn_widget: (widget {type: 'grid name: "myname"})
// Takes a dict with 'type and 'name keys, returns external object wrapping widget
static obj_p fn_widget(obj_p* x, i64_t n) {
    if (n != 1) {
        return ray_err("widget: expects 1 argument (config dict)");
    }

    obj_p config = x[0];
    if (config->type != TYPE_DICT) {
        return ray_err("widget: argument must be a dict");
    }

    // Extract 'type symbol from config
    obj_p type_val = at_sym(config, "type", 4);
    if (!type_val || type_val->type != -TYPE_SYMBOL) {
        if (type_val) drop_obj(type_val);
        return ray_err("widget: missing or invalid 'type (expected symbol)");
    }

    // Extract 'name string from config
    obj_p name_val = at_sym(config, "name", 4);
    if (!name_val || name_val->type != TYPE_C8) {
        drop_obj(type_val);
        if (name_val) drop_obj(name_val);
        return ray_err("widget: missing or invalid 'name (expected string)");
    }

    // Map symbol to widget type
    raygui_widget_type_t wtype;
    if (!widget_type_from_symbol(type_val->i64, &wtype)) {
        drop_obj(type_val);
        drop_obj(name_val);
        return ray_err("widget: unknown type (expected 'grid, 'chart, 'text, or 'repl)");
    }
    drop_obj(type_val);

    // Get the name string (null-terminated)
    char* name_str = malloc(name_val->len + 1);
    if (!name_str) {
        drop_obj(name_val);
        return ray_err("widget: memory allocation failed");
    }
    memcpy(name_str, AS_C8(name_val), name_val->len);
    name_str[name_val->len] = '\0';
    drop_obj(name_val);

    // Create widget
    raygui_widget_t* w = raygui_widget_create(wtype, name_str);
    free(name_str);

    if (!w) {
        return ray_err("widget: failed to create widget");
    }

    // Check we have context
    if (!g_ctx) {
        raygui_widget_destroy(w);
        return ray_err("widget: no raygui context available");
    }

    // Send WIDGET_CREATED message to UI
    raygui_ray_msg_t* msg = malloc(sizeof(raygui_ray_msg_t));
    if (!msg) {
        raygui_widget_destroy(w);
        return ray_err("widget: failed to allocate message");
    }
    msg->type = RAYGUI_MSG_WIDGET_CREATED;
    msg->widget = w;
    msg->data = NULL;
    msg->text = NULL;

    raygui_queue_push(g_ctx->ray_to_ui, msg);
    glfwPostEmptyEvent();  // Wake UI thread

    // Return external object wrapping widget pointer
    // The widget_drop function is a no-op since UI owns the widget
    return external(w, widget_drop);
}

// fn_draw: (draw widget data)
// widget is external object, data is the data to render
// Returns the widget for chaining
static obj_p fn_draw(obj_p* x, i64_t n) {
    if (n != 2) {
        return ray_err("draw: expects 2 arguments (widget, data)");
    }

    obj_p widget_obj = x[0];
    obj_p data = x[1];

    if (widget_obj->type != TYPE_EXT) {
        return ray_err("draw: first argument must be a widget");
    }

    raygui_widget_t* w = (raygui_widget_t*)widget_obj->obj;
    if (!w) {
        return ray_err("draw: widget is null");
    }

    // Check we have context
    if (!g_ctx) {
        return ray_err("draw: no raygui context available");
    }

    // Clone data for UI (UI will own this copy and drop when done)
    obj_p data_copy = clone_obj(data);

    // Send DRAW message to UI
    raygui_ray_msg_t* msg = malloc(sizeof(raygui_ray_msg_t));
    if (!msg) {
        drop_obj(data_copy);
        return ray_err("draw: failed to allocate message");
    }
    msg->type = RAYGUI_MSG_DRAW;
    msg->widget = w;
    msg->data = data_copy;
    msg->text = NULL;

    raygui_queue_push(g_ctx->ray_to_ui, msg);
    glfwPostEmptyEvent();  // Wake UI thread

    // Return widget for chaining
    return clone_obj(widget_obj);
}

// Macro to register a function into the runtime's function dict
// Based on REGISTER_FN from env.c but adapted for external registration
#define RAYGUI_REGISTER_FN(functions, name, fn_type, flags, fn_ptr)   \
    {                                                                  \
        i64_t _k = symbols_intern(name, strlen(name));                \
        push_raw(&AS_LIST(functions)[0], &_k);                        \
        obj_p _o = atom(-(fn_type));                                  \
        _o->attrs = (flags) | ATTR_PROTECTED;                         \
        _o->i64 = (i64_t)(fn_ptr);                                    \
        push_raw(&AS_LIST(functions)[1], &_o);                        \
    }

// Register raygui functions with Rayforce runtime
static void register_raygui_functions(void) {
    runtime_p rt = runtime_get();
    if (!rt) return;

    obj_p functions = rt->env.functions;

    // Register widget function: (widget {type: 'grid name: "name"}) -> external
    RAYGUI_REGISTER_FN(functions, "widget", TYPE_VARY, FN_NONE, fn_widget);

    // Register draw function: (draw widget data) -> widget
    RAYGUI_REGISTER_FN(functions, "draw", TYPE_VARY, FN_NONE, fn_draw);
}

void* raygui_rayforce_thread(void* arg) {
    raygui_ctx_t* ctx = (raygui_ctx_t*)arg;
    runtime_p runtime;
    poll_waker_p waker;

    if (!ctx) {
        return NULL;
    }

    // Step 1: Create Rayforce runtime
    runtime = runtime_create(ctx->argc, ctx->argv);
    if (!runtime) {
        // Signal ready anyway so UI thread doesn't hang
        raygui_ctx_set_quit(ctx, B8_TRUE);
        raygui_ctx_signal_ready(ctx);
        return NULL;
    }

    // Step 2: Set thread-local context for raygui functions
    g_ctx = ctx;

    // Step 3: Register raygui extension types
    register_raygui_types();

    // Step 4: Register raygui functions (widget, draw)
    register_raygui_functions();

    // Step 5: Create poll waker for UI messages
    waker = poll_waker_create(runtime->poll, on_ui_message, ctx);
    if (!waker) {
        g_ctx = NULL;
        runtime_destroy();
        raygui_ctx_set_quit(ctx, B8_TRUE);
        raygui_ctx_signal_ready(ctx);
        return NULL;
    }

    // Step 6: Store waker in context
    raygui_ctx_set_waker(ctx, waker);

    // Step 7: Signal ready
    raygui_ctx_signal_ready(ctx);

    // Step 8: Run poll loop (blocks until exit)
    runtime_run();

    // Step 9: Cleanup
    g_ctx = NULL;
    // Waker is a separate allocation - must be explicitly destroyed
    raygui_ctx_set_waker(ctx, NULL);
    poll_waker_destroy(waker);
    runtime_destroy();

    return NULL;
}
