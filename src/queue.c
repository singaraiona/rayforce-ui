// src/queue.c
#include "../include/rfui/queue.h"
#include <stdlib.h>

rfui_queue_p rfui_queue_create(i64_t capacity) {
    rfui_queue_p q = malloc(sizeof(struct rfui_queue_t));
    if (!q) return NULL;

    q->data = malloc(sizeof(raw_p) * capacity);
    if (!q->data) {
        free(q);
        return NULL;
    }

    q->capacity = capacity;
    q->head = 0;
    q->tail = 0;
    q->mutex = mutex_create();

    return q;
}

nil_t rfui_queue_destroy(rfui_queue_p q) {
    if (!q) return;
    mutex_destroy(&q->mutex);
    free(q->data);
    free(q);
}

b8_t rfui_queue_push(rfui_queue_p q, raw_p item) {
    mutex_lock(&q->mutex);

    i64_t next = (q->tail + 1) % q->capacity;
    if (next == q->head) {
        mutex_unlock(&q->mutex);
        return B8_FALSE; // Full
    }

    q->data[q->tail] = item;
    q->tail = next;

    mutex_unlock(&q->mutex);
    return B8_TRUE;
}

raw_p rfui_queue_pop(rfui_queue_p q) {
    mutex_lock(&q->mutex);

    if (q->head == q->tail) {
        mutex_unlock(&q->mutex);
        return NULL; // Empty
    }

    raw_p item = q->data[q->head];
    q->head = (q->head + 1) % q->capacity;

    mutex_unlock(&q->mutex);
    return item;
}

b8_t rfui_queue_empty(rfui_queue_p q) {
    mutex_lock(&q->mutex);
    b8_t empty = (q->head == q->tail);
    mutex_unlock(&q->mutex);
    return empty;
}
