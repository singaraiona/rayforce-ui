// include/rfui/queue.h
#ifndef RFUI_QUEUE_H
#define RFUI_QUEUE_H

#include "../../deps/rayforce/core/rayforce.h"
#include <pthread.h>

typedef struct rfui_queue_t {
    raw_p* data;
    i64_t capacity;
    i64_t head;
    i64_t tail;
    pthread_mutex_t mutex;
} *rfui_queue_p;

rfui_queue_p rfui_queue_create(i64_t capacity);
nil_t rfui_queue_destroy(rfui_queue_p q);
b8_t rfui_queue_push(rfui_queue_p q, raw_p item);
raw_p rfui_queue_pop(rfui_queue_p q);
b8_t rfui_queue_empty(rfui_queue_p q);

#endif // RFUI_QUEUE_H
