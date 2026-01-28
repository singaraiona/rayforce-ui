// src/queue.c
#include "../include/raygui/queue.h"
#include <stdlib.h>

raygui_queue_p raygui_queue_create(i64_t capacity) {
    raygui_queue_p q = malloc(sizeof(struct raygui_queue_t));
    if (!q) return NULL;

    q->data = malloc(sizeof(raw_p) * capacity);
    if (!q->data) {
        free(q);
        return NULL;
    }

    q->capacity = capacity;
    q->head = 0;
    q->tail = 0;
    pthread_mutex_init(&q->mutex, NULL);

    return q;
}

nil_t raygui_queue_destroy(raygui_queue_p q) {
    if (!q) return;
    pthread_mutex_destroy(&q->mutex);
    free(q->data);
    free(q);
}

b8_t raygui_queue_push(raygui_queue_p q, raw_p item) {
    pthread_mutex_lock(&q->mutex);

    i64_t next = (q->tail + 1) % q->capacity;
    if (next == q->head) {
        pthread_mutex_unlock(&q->mutex);
        return B8_FALSE; // Full
    }

    q->data[q->tail] = item;
    q->tail = next;

    pthread_mutex_unlock(&q->mutex);
    return B8_TRUE;
}

raw_p raygui_queue_pop(raygui_queue_p q) {
    pthread_mutex_lock(&q->mutex);

    if (q->head == q->tail) {
        pthread_mutex_unlock(&q->mutex);
        return NULL; // Empty
    }

    raw_p item = q->data[q->head];
    q->head = (q->head + 1) % q->capacity;

    pthread_mutex_unlock(&q->mutex);
    return item;
}

b8_t raygui_queue_empty(raygui_queue_p q) {
    pthread_mutex_lock(&q->mutex);
    b8_t empty = (q->head == q->tail);
    pthread_mutex_unlock(&q->mutex);
    return empty;
}
