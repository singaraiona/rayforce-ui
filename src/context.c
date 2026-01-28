// src/context.c
#include <stdlib.h>
#include "../include/raygui/context.h"

raygui_ctx_t* raygui_ctx_create(i32_t argc, str_p argv[]) {
    raygui_ctx_t* ctx = calloc(1, sizeof(raygui_ctx_t));
    if (!ctx) return NULL;

    // Store command line arguments
    ctx->argc = argc;
    ctx->argv = argv;

    // Create queues
    ctx->ui_to_ray = raygui_queue_create(RAYGUI_QUEUE_CAPACITY);
    if (!ctx->ui_to_ray) {
        free(ctx);
        return NULL;
    }

    ctx->ray_to_ui = raygui_queue_create(RAYGUI_QUEUE_CAPACITY);
    if (!ctx->ray_to_ui) {
        raygui_queue_destroy(ctx->ui_to_ray);
        free(ctx);
        return NULL;
    }

    // Initialize thread synchronization primitives
    if (pthread_mutex_init(&ctx->ready_mutex, NULL) != 0) {
        raygui_queue_destroy(ctx->ray_to_ui);
        raygui_queue_destroy(ctx->ui_to_ray);
        free(ctx);
        return NULL;
    }

    if (pthread_cond_init(&ctx->ready_cond, NULL) != 0) {
        pthread_mutex_destroy(&ctx->ready_mutex);
        raygui_queue_destroy(ctx->ray_to_ui);
        raygui_queue_destroy(ctx->ui_to_ray);
        free(ctx);
        return NULL;
    }

    // Initialize state
    ctx->ready = B8_FALSE;
    ctx->quit = B8_FALSE;
    ctx->waker = NULL;

    return ctx;
}

nil_t raygui_ctx_destroy(raygui_ctx_t* ctx) {
    if (!ctx) return;

    // Destroy synchronization primitives
    pthread_cond_destroy(&ctx->ready_cond);
    pthread_mutex_destroy(&ctx->ready_mutex);

    // Destroy queues
    raygui_queue_destroy(ctx->ui_to_ray);
    raygui_queue_destroy(ctx->ray_to_ui);

    // Note: waker is owned by poll, so we don't destroy it here

    free(ctx);
}

nil_t raygui_ctx_wait_ready(raygui_ctx_t* ctx) {
    if (!ctx) return;

    pthread_mutex_lock(&ctx->ready_mutex);
    while (!ctx->ready) {
        pthread_cond_wait(&ctx->ready_cond, &ctx->ready_mutex);
    }
    pthread_mutex_unlock(&ctx->ready_mutex);
}

nil_t raygui_ctx_signal_ready(raygui_ctx_t* ctx) {
    if (!ctx) return;

    pthread_mutex_lock(&ctx->ready_mutex);
    ctx->ready = B8_TRUE;
    pthread_cond_signal(&ctx->ready_cond);
    pthread_mutex_unlock(&ctx->ready_mutex);
}
