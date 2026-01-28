// src/main.c
// Note: def.h is force-included via -include flag
#include "../include/raygui/raygui.h"
#include "../include/raygui/context.h"
#include "../include/raygui/message.h"
#include "../include/raygui/queue.h"
#include "../include/raygui/rayforce_thread.h"
#include "../include/raygui/ui.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

// C standard library string functions (declared explicitly since rayforce's
// string.h shadows <string.h> in the include path)
extern size_t strlen(const char* s);
extern void* memcpy(void* dest, const void* src, size_t n);

// Helper to duplicate string (portable version of strdup)
static char* raygui_strdup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* dup = malloc(len);
    if (dup) {
        memcpy(dup, s, len);
    }
    return dup;
}

// Global state (g_ctx is non-static so ui.cpp can access it)
raygui_ctx_t* g_ctx = NULL;
static pthread_t g_ray_thread;

i32_t raygui_init(i32_t argc, str_p argv[]) {
    // Create context with command line arguments
    g_ctx = raygui_ctx_create(argc, argv);
    if (!g_ctx) {
        fprintf(stderr, "Failed to create raygui context\n");
        return -1;
    }

    // Initialize UI (GLFW/ImGui)
    if (raygui_ui_init() != 0) {
        fprintf(stderr, "Failed to initialize UI\n");
        raygui_ctx_destroy(g_ctx);
        g_ctx = NULL;
        return -1;
    }

    // Start Rayforce thread
    if (pthread_create(&g_ray_thread, NULL, raygui_rayforce_thread, g_ctx) != 0) {
        fprintf(stderr, "Failed to create Rayforce thread\n");
        raygui_ui_destroy();
        raygui_ctx_destroy(g_ctx);
        g_ctx = NULL;
        return -1;
    }

    // Wait for Rayforce thread to signal ready
    raygui_ctx_wait_ready(g_ctx);

    return 0;
}

i32_t raygui_eval(const char* expr) {
    if (!g_ctx || !expr) {
        return -1;
    }

    // Create MSG_EVAL message with strdup'd expression
    raygui_ui_msg_t* msg = malloc(sizeof(raygui_ui_msg_t));
    if (!msg) {
        return -1;
    }

    msg->type = RAYGUI_MSG_EVAL;
    msg->expr = raygui_strdup(expr);
    msg->obj = NULL;
    msg->widget = NULL;

    if (!msg->expr) {
        free(msg);
        return -1;
    }

    // Push to ui_to_ray queue
    if (!raygui_queue_push(g_ctx->ui_to_ray, msg)) {
        free(msg->expr);
        free(msg);
        return -1;
    }

    // Wake Rayforce thread
    poll_waker_p waker = raygui_ctx_get_waker(g_ctx);
    if (waker) {
        poll_waker_wake(waker);
    }

    return 0;
}

i32_t raygui_run(nil_t) {
    if (!g_ctx) {
        return 1;
    }

    // Run the ImGui/GLFW main loop
    return raygui_ui_run();
}

nil_t raygui_destroy(nil_t) {
    if (!g_ctx) {
        return;
    }

    // Send MSG_QUIT to Rayforce thread
    raygui_ui_msg_t* quit_msg = malloc(sizeof(raygui_ui_msg_t));
    if (quit_msg) {
        quit_msg->type = RAYGUI_MSG_QUIT;
        quit_msg->expr = NULL;
        quit_msg->obj = NULL;
        quit_msg->widget = NULL;

        raygui_queue_push(g_ctx->ui_to_ray, quit_msg);

        // Wake the thread
        poll_waker_p waker = raygui_ctx_get_waker(g_ctx);
        if (waker) {
            poll_waker_wake(waker);
        }
    } else {
        // Malloc failed - set quit flag directly so Rayforce thread can still exit
        fprintf(stderr, "Warning: Failed to allocate quit message, setting quit flag directly\n");
        raygui_ctx_set_quit(g_ctx, 1);

        // Wake the thread so it can see the quit flag
        poll_waker_p waker = raygui_ctx_get_waker(g_ctx);
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
    raygui_ui_destroy();

    // Destroy context
    raygui_ctx_destroy(g_ctx);
    g_ctx = NULL;
}

i32_t main(i32_t argc, str_p argv[]) {
    printf("raygui v%d.%d\n", RAYGUI_VERSION_MAJOR, RAYGUI_VERSION_MINOR);

    if (raygui_init(argc, argv) != 0) {
        return -1;
    }

    i32_t code = raygui_run();
    raygui_destroy();

    return code;
}
