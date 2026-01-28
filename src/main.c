// src/main.c
// Note: def.h is force-included via -include flag
#include "../include/rfui/rfui.h"
#include "../include/rfui/context.h"
#include "../include/rfui/message.h"
#include "../include/rfui/queue.h"
#include "../include/rfui/rayforce_thread.h"
#include "../include/rfui/ui.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

// C standard library string functions (declared explicitly since rayforce's
// string.h shadows <string.h> in the include path)
extern size_t strlen(const char* s);
extern void* memcpy(void* dest, const void* src, size_t n);

// Helper to duplicate string (portable version of strdup)
static char* rfui_strdup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* dup = malloc(len);
    if (dup) {
        memcpy(dup, s, len);
    }
    return dup;
}

// Global state (g_ctx is non-static so ui.cpp can access it)
rfui_ctx_t* g_ctx = NULL;
static pthread_t g_ray_thread;

i32_t rfui_init(i32_t argc, str_p argv[]) {
    // Create context with command line arguments
    g_ctx = rfui_ctx_create(argc, argv);
    if (!g_ctx) {
        fprintf(stderr, "Failed to create rayforce-ui context\n");
        return -1;
    }

    // Initialize UI (GLFW/ImGui)
    if (rfui_ui_init() != 0) {
        fprintf(stderr, "Failed to initialize UI\n");
        rfui_ctx_destroy(g_ctx);
        g_ctx = NULL;
        return -1;
    }

    // Start Rayforce thread
    if (pthread_create(&g_ray_thread, NULL, rfui_rayforce_thread, g_ctx) != 0) {
        fprintf(stderr, "Failed to create Rayforce thread\n");
        rfui_ui_destroy();
        rfui_ctx_destroy(g_ctx);
        g_ctx = NULL;
        return -1;
    }

    // Wait for Rayforce thread to signal ready
    rfui_ctx_wait_ready(g_ctx);

    return 0;
}

i32_t rfui_eval(const char* expr) {
    if (!g_ctx || !expr) {
        return -1;
    }

    // Create MSG_EVAL message with strdup'd expression
    rfui_ui_msg_t* msg = malloc(sizeof(rfui_ui_msg_t));
    if (!msg) {
        return -1;
    }

    msg->type = RFUI_MSG_EVAL;
    msg->expr = rfui_strdup(expr);
    msg->obj = NULL;
    msg->widget = NULL;

    if (!msg->expr) {
        free(msg);
        return -1;
    }

    // Push to ui_to_ray queue
    if (!rfui_queue_push(g_ctx->ui_to_ray, msg)) {
        free(msg->expr);
        free(msg);
        return -1;
    }

    // Wake Rayforce thread
    poll_waker_p waker = rfui_ctx_get_waker(g_ctx);
    if (waker) {
        poll_waker_wake(waker);
    }

    return 0;
}

i32_t rfui_run(nil_t) {
    if (!g_ctx) {
        return 1;
    }

    // Run the ImGui/GLFW main loop
    return rfui_ui_run();
}

nil_t rfui_destroy(nil_t) {
    if (!g_ctx) {
        return;
    }

    // Send MSG_QUIT to Rayforce thread
    rfui_ui_msg_t* quit_msg = malloc(sizeof(rfui_ui_msg_t));
    if (quit_msg) {
        quit_msg->type = RFUI_MSG_QUIT;
        quit_msg->expr = NULL;
        quit_msg->obj = NULL;
        quit_msg->widget = NULL;

        rfui_queue_push(g_ctx->ui_to_ray, quit_msg);

        // Wake the thread
        poll_waker_p waker = rfui_ctx_get_waker(g_ctx);
        if (waker) {
            poll_waker_wake(waker);
        }
    } else {
        // Malloc failed - set quit flag directly so Rayforce thread can still exit
        fprintf(stderr, "Warning: Failed to allocate quit message, setting quit flag directly\n");
        rfui_ctx_set_quit(g_ctx, 1);

        // Wake the thread so it can see the quit flag
        poll_waker_p waker = rfui_ctx_get_waker(g_ctx);
        if (waker) {
            poll_waker_wake(waker);
        }
    }

    // Join thread - continue with cleanup even if join fails
    int join_result = pthread_join(g_ray_thread, NULL);
    if (join_result != 0) {
        fprintf(stderr, "Warning: pthread_join failed with error %d\n", join_result);
    }

    // Destroy UI (GLFW/ImGui)
    rfui_ui_destroy();

    // Destroy context
    rfui_ctx_destroy(g_ctx);
    g_ctx = NULL;
}

i32_t main(i32_t argc, str_p argv[]) {
    printf("rayforce-ui v%d.%d\n", RFUI_VERSION_MAJOR, RFUI_VERSION_MINOR);

    if (rfui_init(argc, argv) != 0) {
        return -1;
    }

    // Note: REPL widget is created by Rayforce thread after runtime starts

    i32_t code = rfui_run();
    rfui_destroy();

    return code;
}
