// src/context.c
#include <stdlib.h>
#include "../include/rfui/context.h"

rfui_ctx_t* rfui_ctx_create(i32_t argc, str_p argv[]) {
    rfui_ctx_t* ctx = calloc(1, sizeof(rfui_ctx_t));
    if (!ctx) return NULL;

    // Store command line arguments (shallow copy)
    // NOTE: Caller must ensure argv remains valid for context lifetime
    ctx->argc = argc;
    ctx->argv = argv;

    // Create queues
    ctx->ui_to_ray = rfui_queue_create(RFUI_QUEUE_CAPACITY);
    if (!ctx->ui_to_ray) {
        free(ctx);
        return NULL;
    }

    ctx->ray_to_ui = rfui_queue_create(RFUI_QUEUE_CAPACITY);
    if (!ctx->ray_to_ui) {
        rfui_queue_destroy(ctx->ui_to_ray);
        free(ctx);
        return NULL;
    }

    // Initialize thread synchronization primitives
    if (pthread_mutex_init(&ctx->ready_mutex, NULL) != 0) {
        rfui_queue_destroy(ctx->ray_to_ui);
        rfui_queue_destroy(ctx->ui_to_ray);
        free(ctx);
        return NULL;
    }

    if (pthread_cond_init(&ctx->ready_cond, NULL) != 0) {
        pthread_mutex_destroy(&ctx->ready_mutex);
        rfui_queue_destroy(ctx->ray_to_ui);
        rfui_queue_destroy(ctx->ui_to_ray);
        free(ctx);
        return NULL;
    }

    // Initialize state
    ctx->ready = B8_FALSE;
    ctx->quit = B8_FALSE;
    ctx->waker = NULL;

    return ctx;
}

nil_t rfui_ctx_destroy(rfui_ctx_t* ctx) {
    if (!ctx) return;

    // Destroy synchronization primitives
    pthread_cond_destroy(&ctx->ready_cond);
    pthread_mutex_destroy(&ctx->ready_mutex);

    // Destroy queues
    rfui_queue_destroy(ctx->ui_to_ray);
    rfui_queue_destroy(ctx->ray_to_ui);

    // Note: waker is owned by poll, so we don't destroy it here

    free(ctx);
}

nil_t rfui_ctx_wait_ready(rfui_ctx_t* ctx) {
    if (!ctx) return;

    if (pthread_mutex_lock(&ctx->ready_mutex) != 0) return;
    while (!ctx->ready) {
        pthread_cond_wait(&ctx->ready_cond, &ctx->ready_mutex);
    }
    (void)pthread_mutex_unlock(&ctx->ready_mutex);
}

nil_t rfui_ctx_signal_ready(rfui_ctx_t* ctx) {
    if (!ctx) return;

    if (pthread_mutex_lock(&ctx->ready_mutex) != 0) return;
    ctx->ready = B8_TRUE;
    (void)pthread_cond_signal(&ctx->ready_cond);
    (void)pthread_mutex_unlock(&ctx->ready_mutex);
}

nil_t rfui_ctx_set_quit(rfui_ctx_t* ctx, b8_t quit) {
    if (!ctx) return;

    if (pthread_mutex_lock(&ctx->ready_mutex) != 0) return;
    ctx->quit = quit;
    (void)pthread_mutex_unlock(&ctx->ready_mutex);
}

b8_t rfui_ctx_get_quit(rfui_ctx_t* ctx) {
    if (!ctx) return B8_TRUE;  // Safe default: quit if ctx is invalid

    if (pthread_mutex_lock(&ctx->ready_mutex) != 0) return B8_TRUE;
    b8_t quit = ctx->quit;
    (void)pthread_mutex_unlock(&ctx->ready_mutex);
    return quit;
}

nil_t rfui_ctx_set_waker(rfui_ctx_t* ctx, poll_waker_p waker) {
    if (!ctx) return;

    if (pthread_mutex_lock(&ctx->ready_mutex) != 0) return;
    ctx->waker = waker;
    (void)pthread_mutex_unlock(&ctx->ready_mutex);
}

poll_waker_p rfui_ctx_get_waker(rfui_ctx_t* ctx) {
    if (!ctx) return NULL;

    if (pthread_mutex_lock(&ctx->ready_mutex) != 0) return NULL;
    poll_waker_p waker = ctx->waker;
    (void)pthread_mutex_unlock(&ctx->ready_mutex);
    return waker;
}
