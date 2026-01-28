// include/raygui/context.h
#ifndef RAYGUI_CONTEXT_H
#define RAYGUI_CONTEXT_H

#include "../../deps/rayforce/core/rayforce.h"
#include "../../deps/rayforce/core/poll.h"
#include "queue.h"
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

// Default queue capacity for UI<->Rayforce communication
#define RAYGUI_QUEUE_CAPACITY 1024

typedef struct raygui_ctx_t {
    // Command line args (for runtime_create)
    // NOTE: argv is a shallow copy - caller must ensure argv remains valid
    // for the entire lifetime of this context
    i32_t argc;
    str_p* argv;

    // Queues
    raygui_queue_p ui_to_ray;   // UI thread -> Rayforce thread
    raygui_queue_p ray_to_ui;   // Rayforce thread -> UI thread

    // Thread sync - protects ready, quit, and waker fields
    pthread_mutex_t ready_mutex;
    pthread_cond_t ready_cond;
    b8_t ready;                  // Rayforce thread signals when ready

    // Rayforce poll waker (set by Rayforce thread)
    // Access must be protected by ready_mutex
    poll_waker_p waker;

    // Exit flag - access must be protected by ready_mutex
    b8_t quit;
} raygui_ctx_t;

// Create a new context with command line arguments
// NOTE: argv is stored as a shallow copy - caller must ensure argv remains
// valid for the entire lifetime of this context (typically program lifetime)
raygui_ctx_t* raygui_ctx_create(i32_t argc, str_p argv[]);

// Destroy context and free all resources
nil_t raygui_ctx_destroy(raygui_ctx_t* ctx);

// Wait for Rayforce thread to signal ready (called by UI thread)
nil_t raygui_ctx_wait_ready(raygui_ctx_t* ctx);

// Signal that Rayforce thread is ready (called by Rayforce thread)
nil_t raygui_ctx_signal_ready(raygui_ctx_t* ctx);

// Thread-safe quit flag accessors
nil_t raygui_ctx_set_quit(raygui_ctx_t* ctx, b8_t quit);
b8_t raygui_ctx_get_quit(raygui_ctx_t* ctx);

// Thread-safe waker accessors
nil_t raygui_ctx_set_waker(raygui_ctx_t* ctx, poll_waker_p waker);
poll_waker_p raygui_ctx_get_waker(raygui_ctx_t* ctx);

#ifdef __cplusplus
}
#endif

#endif // RAYGUI_CONTEXT_H
