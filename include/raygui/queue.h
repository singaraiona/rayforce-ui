// include/raygui/queue.h
#ifndef RAYGUI_QUEUE_H
#define RAYGUI_QUEUE_H

#include "../../deps/rayforce/core/rayforce.h"
#include <pthread.h>

typedef struct raygui_queue_t {
    raw_p* data;
    i64_t capacity;
    i64_t head;
    i64_t tail;
    pthread_mutex_t mutex;
} *raygui_queue_p;

raygui_queue_p raygui_queue_create(i64_t capacity);
nil_t raygui_queue_destroy(raygui_queue_p q);
b8_t raygui_queue_push(raygui_queue_p q, raw_p item);
raw_p raygui_queue_pop(raygui_queue_p q);
b8_t raygui_queue_empty(raygui_queue_p q);

#endif // RAYGUI_QUEUE_H
