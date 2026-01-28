// src/rayforce_thread.c
#include <stdlib.h>
#include "../deps/rayforce/core/runtime.h"
#include "../deps/rayforce/core/poll.h"
#include "../include/raygui/context.h"
#include "../include/raygui/message.h"
#include "../include/raygui/queue.h"
#include "../include/raygui/rayforce_thread.h"

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
            // TODO: Drop obj_p after render
            // For now, stub - would call drop_obj(msg->obj) when implemented
            break;

        case RAYGUI_MSG_QUIT:
            // Set quit flag
            raygui_ctx_set_quit(ctx, B8_TRUE);
            // Exit the poll loop
            if (runtime_get()) {
                poll_exit(runtime_get()->poll, 0);
            }
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

    // Drain the queue and process all pending messages
    while ((msg = (raygui_ui_msg_t*)raygui_queue_pop(ctx->ui_to_ray)) != NULL) {
        process_ui_message(ctx, msg);
    }
}

// Register raygui extension types (stub for now)
static void register_raygui_types(void) {
    // TODO: Register widget type with Rayforce
    // This will be implemented when we add widget rendering
}

// Register raygui functions (stub for now)
static void register_raygui_functions(void) {
    // TODO: Register widget and draw functions with Rayforce
    // This will be implemented when we add widget rendering
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

    // Step 2: Register raygui extension types (stub for now)
    register_raygui_types();

    // Step 3: Register raygui functions (stub for now)
    register_raygui_functions();

    // Step 4: Create poll waker for UI messages
    waker = poll_waker_create(runtime->poll, on_ui_message, ctx);
    if (!waker) {
        runtime_destroy();
        raygui_ctx_set_quit(ctx, B8_TRUE);
        raygui_ctx_signal_ready(ctx);
        return NULL;
    }

    // Step 5: Store waker in context
    raygui_ctx_set_waker(ctx, waker);

    // Step 6: Signal ready
    raygui_ctx_signal_ready(ctx);

    // Step 7: Run poll loop (blocks until exit)
    runtime_run();

    // Step 8: Cleanup
    // Note: waker is destroyed by poll_destroy, but we set it to NULL first
    raygui_ctx_set_waker(ctx, NULL);
    poll_waker_destroy(waker);
    runtime_destroy();

    return NULL;
}
