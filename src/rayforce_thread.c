// src/rayforce_thread.c
// Include system string.h via absolute path to avoid conflict with deps/rayforce/core/string.h
#include "/usr/include/string.h"
#include <stdlib.h>
#include <stdio.h>
#include "../deps/rayforce/core/runtime.h"
#include "../deps/rayforce/core/poll.h"
#include "../deps/rayforce/core/symbols.h"
#include "../deps/rayforce/core/ops.h"
#include "../deps/rayforce/core/util.h"
#include "../deps/rayforce/core/dynlib.h"  // For ext_t (external object structure)
#include "../deps/rayforce/core/io.h"      // For ray_load (script execution)
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
            if (msg->expr) {
                // Evaluate expression
                obj_p result = eval_str(msg->expr);

                // Format result for display
                char* result_text = NULL;
                if (result) {
                    obj_p fmt = obj_fmt(result, B8_TRUE);
                    if (fmt && fmt->type == TYPE_C8) {
                        // Copy formatted string
                        result_text = (char*)malloc(fmt->len + 1);
                        if (result_text) {
                            memcpy(result_text, AS_C8(fmt), fmt->len);
                            result_text[fmt->len] = '\0';
                        }
                        drop_obj(fmt);
                    }
                    drop_obj(result);
                }

                // Send result back to UI
                if (result_text) {
                    raygui_ray_msg_t* reply = (raygui_ray_msg_t*)malloc(sizeof(raygui_ray_msg_t));
                    if (reply) {
                        reply->type = RAYGUI_MSG_RESULT;
                        reply->widget = NULL;
                        reply->data = NULL;
                        reply->text = result_text;

                        if (raygui_queue_push(ctx->ray_to_ui, reply)) {
                            glfwPostEmptyEvent();  // Wake UI thread
                        } else {
                            free(result_text);
                            free(reply);
                        }
                    } else {
                        free(result_text);
                    }
                }

                free(msg->expr);
            }
            break;

        case RAYGUI_MSG_SET_POST_QUERY:
            if (!msg->widget) {
                // Null widget - nothing to do
                if (msg->expr) free(msg->expr);
                break;
            }
            if (msg->expr) {
                // Parse the expression string into an obj_p
                obj_p query = parse_str(msg->expr);
                if (query && !IS_ERR(query)) {
                    // Drop old post_query if exists
                    if (msg->widget->post_query) {
                        drop_obj(msg->widget->post_query);
                    }
                    msg->widget->post_query = query;
                } else {
                    // Parse failed - drop the error result if any
                    if (query) drop_obj(query);
                }
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

    // Fail fast: check context before doing any work
    if (!g_ctx) {
        return ray_err("widget: no raygui context available");
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
//
// KNOWN TECHNICAL DEBT: Widget lifecycle race condition
// The widget pointer validity relies on the assumption that the UI thread
// will not destroy the widget while we hold a reference to it. This is safe
// under the current design where widgets live for the session lifetime and
// are only destroyed during shutdown. A proper fix would require either:
// - Widget IDs with lookup validation on the UI side, or
// - Reference counting with atomic operations
// For now, we assume widgets are never destroyed mid-session.
static obj_p fn_draw(obj_p* x, i64_t n) {
    if (n != 2) {
        return ray_err("draw: expects 2 arguments (widget, data)");
    }

    obj_p widget_obj = x[0];
    obj_p data = x[1];

    if (widget_obj->type != TYPE_EXT) {
        return ray_err("draw: first argument must be a widget");
    }

    // Extract widget pointer from external object
    // TYPE_EXT stores data in ext_t structure: { raw_p ptr; nil_t (*drop)(raw_p); }
    ext_p ext = (ext_p)AS_C8(widget_obj);
    raygui_widget_t* w = (raygui_widget_t*)ext->ptr;
    if (!w) {
        return ray_err("draw: widget is null");
    }

    // Check we have context
    if (!g_ctx) {
        return ray_err("draw: no raygui context available");
    }

    // Apply post_query transformation if widget has one
    // The post_query is a lambda/function that takes data as argument.
    // We build a call expression (post_query data) and evaluate it.
    obj_p final_data;
    if (w->post_query) {
        // Clone the post_query and data for the call expression
        obj_p query_clone = clone_obj(w->post_query);
        if (!query_clone) {
            return ray_err("draw: failed to clone post_query");
        }
        obj_p data_clone = clone_obj(data);
        if (!data_clone) {
            drop_obj(query_clone);
            return ray_err("draw: failed to clone data for post_query");
        }

        // Build call expression: (post_query data)
        // vn_list creates a 2-element list that, when evaluated, calls the function
        obj_p call_expr = vn_list(2, query_clone, data_clone);
        if (!call_expr) {
            drop_obj(query_clone);
            drop_obj(data_clone);
            return ray_err("draw: failed to build call expression");
        }

        // eval_obj consumes the call expression and returns the result
        obj_p result = eval_obj(call_expr);
        if (result && !IS_ERR(result)) {
            final_data = result;
        } else {
            // Query failed, fall back to original data
            if (result) drop_obj(result);
            final_data = clone_obj(data);
            if (!final_data) {
                return ray_err("draw: failed to clone data after query error");
            }
        }
    } else {
        // No post_query, clone data directly
        final_data = clone_obj(data);
        if (!final_data) {
            return ray_err("draw: failed to clone data");
        }
    }

    // Send DRAW message to UI
    raygui_ray_msg_t* msg = malloc(sizeof(raygui_ray_msg_t));
    if (!msg) {
        drop_obj(final_data);
        return ray_err("draw: failed to allocate message");
    }
    msg->type = RAYGUI_MSG_DRAW;
    msg->widget = w;
    msg->text = NULL;

    // For text widgets, pre-format on Rayforce thread (UI thread has no runtime)
    if (w->type == RAYGUI_WIDGET_TEXT) {
        obj_p fmt = obj_fmt(final_data, B8_TRUE);
        if (fmt && fmt->type == TYPE_C8) {
            msg->text = (char*)malloc(fmt->len + 1);
            if (msg->text) {
                memcpy(msg->text, AS_C8(fmt), fmt->len);
                msg->text[fmt->len] = '\0';
            }
            drop_obj(fmt);
        }
        // Drop the obj_p data here since text widget uses pre-formatted string
        drop_obj(final_data);
        msg->data = NULL;
    } else {
        msg->data = final_data;
    }

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

    // Step 5: Load script file if provided via command line
    {
        obj_p file_arg = runtime_get_arg("file");
        if (!is_null(file_arg)) {
            obj_p res = ray_load(file_arg);
            drop_obj(file_arg);
            if (IS_ERR(res)) {
                // Print error but continue - GUI still runs
                obj_p fmt = obj_fmt(res, B8_TRUE);
                if (fmt) {
                    fprintf(stderr, "Script error: %.*s\n", (i32_t)fmt->len, AS_C8(fmt));
                    drop_obj(fmt);
                }
            }
            drop_obj(res);
        }
    }

    // Step 6: Create poll waker for UI messages
    waker = poll_waker_create(runtime->poll, on_ui_message, ctx);
    if (!waker) {
        g_ctx = NULL;
        runtime_destroy();
        raygui_ctx_set_quit(ctx, B8_TRUE);
        raygui_ctx_signal_ready(ctx);
        return NULL;
    }

    // Step 7: Store waker in context
    raygui_ctx_set_waker(ctx, waker);

    // Step 8: Signal ready
    raygui_ctx_signal_ready(ctx);

    // Step 9: Create default REPL widget
    {
        obj_p repl_result = eval_str("(set *repl* (widget {type: 'repl name: \"REPL\"}))");
        if (repl_result) {
            drop_obj(repl_result);
        }
    }

    // Step 10: Run poll loop (blocks until exit)
    runtime_run();

    // Cleanup
    g_ctx = NULL;
    // Waker is a separate allocation - must be explicitly destroyed
    raygui_ctx_set_waker(ctx, NULL);
    poll_waker_destroy(waker);
    runtime_destroy();

    return NULL;
}
