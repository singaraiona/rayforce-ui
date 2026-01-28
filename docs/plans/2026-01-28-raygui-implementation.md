# raygui Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Build a GUI framework for RayforceDB with zero-copy rendering and REPL-centric interaction.

**Architecture:** UI runs on main thread (GLFW/ImGui), Rayforce runtime in worker thread. Communication via queues with cross-platform wakers. Widgets are first-class Rayforce external types.

**Tech Stack:** C17, Rayforce, Dear ImGui (docking branch), ImPlot, GLFW, OpenGL 3.3+

---

## Task 1: Project Structure & Build System

**Files:**
- Create: `Makefile`
- Create: `include/raygui/raygui.h`
- Create: `src/main.c`

**Step 1: Create directory structure**

```bash
mkdir -p include/raygui src
```

**Step 2: Create main header**

```c
// include/raygui/raygui.h
#ifndef RAYGUI_H
#define RAYGUI_H

#include "../../deps/rayforce/core/rayforce.h"
#include "../../deps/rayforce/core/runtime.h"
#include "../../deps/rayforce/core/poll.h"

#ifdef __cplusplus
extern "C" {
#endif

// Version
#define RAYGUI_VERSION_MAJOR 0
#define RAYGUI_VERSION_MINOR 1

// Initialize raygui (call from main thread before starting rayforce thread)
i32_t raygui_init(i32_t argc, str_p argv[]);

// Run main loop (blocks until quit)
i32_t raygui_run(nil_t);

// Cleanup
nil_t raygui_destroy(nil_t);

#ifdef __cplusplus
}
#endif

#endif // RAYGUI_H
```

**Step 3: Create minimal main.c**

```c
// src/main.c
#include "../include/raygui/raygui.h"
#include <stdio.h>

i32_t main(i32_t argc, str_p argv[]) {
    printf("raygui v%d.%d\n", RAYGUI_VERSION_MAJOR, RAYGUI_VERSION_MINOR);

    if (raygui_init(argc, argv) != 0) {
        return -1;
    }

    i32_t code = raygui_run();
    raygui_destroy();

    return code;
}
```

**Step 4: Create Makefile**

```makefile
# Makefile
CC = clang
STD = c17

ifeq ($(OS),)
OS := $(shell uname -s | tr "[:upper:]" "[:lower:]")
endif

# Build rayforce library first
RAYFORCE_DIR = deps/rayforce
RAYFORCE_LIB = $(RAYFORCE_DIR)/librayforce.a

ifeq ($(OS),linux)
CFLAGS = -fPIC -Wall -Wextra -std=$(STD) -g -O0 -march=native -fsigned-char -DDEBUG -m64
LIBS = -lm -ldl -lpthread -lGL -lglfw
endif

ifeq ($(OS),darwin)
CFLAGS = -fPIC -Wall -Wextra -std=$(STD) -g -O0 -march=native -fsigned-char -DDEBUG -m64
LIBS = -lm -ldl -lpthread -framework OpenGL -framework Cocoa -framework IOKit -lglfw
endif

INCLUDES = -Iinclude -I$(RAYFORCE_DIR)/core -Ideps/imgui -Ideps/implot -Ideps/glfw/include

SRC = src/main.c
OBJ = $(SRC:.c=.o)
TARGET = raygui

default: rayforce_lib $(TARGET)

rayforce_lib:
	$(MAKE) -C $(RAYFORCE_DIR) lib-debug

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(RAYFORCE_LIB) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
	$(MAKE) -C $(RAYFORCE_DIR) clean

.PHONY: default clean rayforce_lib
```

**Step 5: Build and verify**

Run: `make`
Expected: Compiles successfully, prints version on run

**Step 6: Commit**

```bash
git add Makefile include/ src/
git commit -m "feat: initial project structure and build system"
```

---

## Task 2: Thread-Safe Queue

**Files:**
- Create: `include/raygui/queue.h`
- Create: `src/queue.c`

**Step 1: Create queue header**

```c
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
```

**Step 2: Implement queue**

```c
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
```

**Step 3: Update Makefile**

Add `src/queue.c` to SRC list.

**Step 4: Build and verify**

Run: `make clean && make`
Expected: Compiles successfully

**Step 5: Commit**

```bash
git add include/raygui/queue.h src/queue.c Makefile
git commit -m "feat: add thread-safe queue for inter-thread communication"
```

---

## Task 3: Message Types

**Files:**
- Create: `include/raygui/message.h`

**Step 1: Create message types**

```c
// include/raygui/message.h
#ifndef RAYGUI_MESSAGE_H
#define RAYGUI_MESSAGE_H

#include "../../deps/rayforce/core/rayforce.h"

// Forward declaration
struct raygui_widget_t;

// UI → Rayforce message types
typedef enum raygui_ui_msg_type_t {
    RAYGUI_MSG_EVAL,           // Evaluate expression
    RAYGUI_MSG_SET_POST_QUERY, // Set widget post_query
    RAYGUI_MSG_DROP,           // Drop obj_p after render
    RAYGUI_MSG_QUIT            // Shutdown
} raygui_ui_msg_type_t;

// Rayforce → UI message types
typedef enum raygui_ray_msg_type_t {
    RAYGUI_MSG_WIDGET_CREATED, // New widget panel
    RAYGUI_MSG_DRAW,           // Widget data update
    RAYGUI_MSG_RESULT          // REPL result
} raygui_ray_msg_type_t;

// UI → Rayforce message
typedef struct raygui_ui_msg_t {
    raygui_ui_msg_type_t type;
    char* expr;                      // Expression string (owned, must free)
    obj_p obj;                       // Object to drop
    struct raygui_widget_t* widget;  // Target widget
} raygui_ui_msg_t;

// Rayforce → UI message
typedef struct raygui_ray_msg_t {
    raygui_ray_msg_type_t type;
    struct raygui_widget_t* widget;  // Target widget
    obj_p data;                      // Data for rendering
    char* text;                      // Result text (owned, must free)
} raygui_ray_msg_t;

#endif // RAYGUI_MESSAGE_H
```

**Step 2: Commit**

```bash
git add include/raygui/message.h
git commit -m "feat: add message types for inter-thread communication"
```

---

## Task 4: Widget Types

**Files:**
- Create: `include/raygui/widget.h`
- Create: `src/widget.c`

**Step 1: Create widget header**

```c
// include/raygui/widget.h
#ifndef RAYGUI_WIDGET_H
#define RAYGUI_WIDGET_H

#include "../../deps/rayforce/core/rayforce.h"

typedef enum raygui_widget_type_t {
    RAYGUI_WIDGET_GRID,
    RAYGUI_WIDGET_CHART,
    RAYGUI_WIDGET_TEXT,
    RAYGUI_WIDGET_REPL
} raygui_widget_type_t;

typedef struct raygui_widget_t {
    raygui_widget_type_t type;
    char* name;
    obj_p data;           // Base data from draw()
    obj_p post_query;     // Expression applied before render
    obj_p on_select;      // Callback function

    // UI state (UI thread only)
    b8_t is_open;
    u32_t dock_id;
    raw_p ui_state;       // Type-specific UI state
    obj_p render_data;    // Current data for rendering
} raygui_widget_t;

// Create widget struct (called from Rayforce thread)
raygui_widget_t* raygui_widget_create(raygui_widget_type_t type, const char* name);

// Destroy widget struct
nil_t raygui_widget_destroy(raygui_widget_t* w);

// Format widget for display
char* raygui_widget_format(raygui_widget_t* w);

// Get type name
const char* raygui_widget_type_name(raygui_widget_type_t type);

#endif // RAYGUI_WIDGET_H
```

**Step 2: Implement widget**

```c
// src/widget.c
#include "../include/raygui/widget.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

const char* raygui_widget_type_name(raygui_widget_type_t type) {
    switch (type) {
        case RAYGUI_WIDGET_GRID:  return "grid";
        case RAYGUI_WIDGET_CHART: return "chart";
        case RAYGUI_WIDGET_TEXT:  return "text";
        case RAYGUI_WIDGET_REPL:  return "repl";
        default: return "unknown";
    }
}

raygui_widget_t* raygui_widget_create(raygui_widget_type_t type, const char* name) {
    raygui_widget_t* w = malloc(sizeof(raygui_widget_t));
    if (!w) return NULL;

    w->type = type;
    w->name = strdup(name);
    w->data = NULL;
    w->post_query = NULL;
    w->on_select = NULL;
    w->is_open = B8_TRUE;
    w->dock_id = 0;
    w->ui_state = NULL;
    w->render_data = NULL;

    return w;
}

nil_t raygui_widget_destroy(raygui_widget_t* w) {
    if (!w) return;

    free(w->name);
    if (w->data) drop_obj(w->data);
    if (w->post_query) drop_obj(w->post_query);
    if (w->on_select) drop_obj(w->on_select);
    if (w->render_data) drop_obj(w->render_data);
    free(w);
}

char* raygui_widget_format(raygui_widget_t* w) {
    char* buf = malloc(256);
    snprintf(buf, 256, "widget<%s:\"%s\">",
             raygui_widget_type_name(w->type), w->name);
    return buf;
}
```

**Step 3: Update Makefile**

Add `src/widget.c` to SRC list.

**Step 4: Build and verify**

Run: `make clean && make`
Expected: Compiles successfully

**Step 5: Commit**

```bash
git add include/raygui/widget.h src/widget.c Makefile
git commit -m "feat: add widget type and basic operations"
```

---

## Task 5: Context & Threading Setup

**Files:**
- Create: `include/raygui/context.h`
- Create: `src/context.c`
- Modify: `src/main.c`

**Step 1: Create context header**

```c
// include/raygui/context.h
#ifndef RAYGUI_CONTEXT_H
#define RAYGUI_CONTEXT_H

#include "../../deps/rayforce/core/rayforce.h"
#include "../../deps/rayforce/core/runtime.h"
#include "../../deps/rayforce/core/poll.h"
#include "queue.h"
#include <pthread.h>

#define RAYGUI_QUEUE_SIZE 1024

typedef struct raygui_ctx_t {
    // Command line args
    i32_t argc;
    str_p* argv;

    // Queues
    raygui_queue_p ui_to_ray;  // UI → Rayforce
    raygui_queue_p ray_to_ui;  // Rayforce → UI

    // Rayforce thread
    pthread_t ray_thread;
    poll_waker_p waker;        // To wake Rayforce from UI

    // Synchronization
    pthread_mutex_t ready_mutex;
    pthread_cond_t ready_cond;
    b8_t ray_ready;
    b8_t should_quit;
} raygui_ctx_t;

// Global context
extern raygui_ctx_t* g_ctx;

// Initialize context
raygui_ctx_t* raygui_ctx_create(i32_t argc, str_p argv[]);

// Destroy context
nil_t raygui_ctx_destroy(raygui_ctx_t* ctx);

// Wait for rayforce thread to be ready
nil_t raygui_ctx_wait_ready(raygui_ctx_t* ctx);

// Signal rayforce is ready
nil_t raygui_ctx_signal_ready(raygui_ctx_t* ctx);

#endif // RAYGUI_CONTEXT_H
```

**Step 2: Implement context**

```c
// src/context.c
#include "../include/raygui/context.h"
#include <stdlib.h>

raygui_ctx_t* g_ctx = NULL;

raygui_ctx_t* raygui_ctx_create(i32_t argc, str_p argv[]) {
    raygui_ctx_t* ctx = malloc(sizeof(raygui_ctx_t));
    if (!ctx) return NULL;

    ctx->argc = argc;
    ctx->argv = argv;

    ctx->ui_to_ray = raygui_queue_create(RAYGUI_QUEUE_SIZE);
    ctx->ray_to_ui = raygui_queue_create(RAYGUI_QUEUE_SIZE);

    if (!ctx->ui_to_ray || !ctx->ray_to_ui) {
        raygui_ctx_destroy(ctx);
        return NULL;
    }

    ctx->waker = NULL;
    ctx->ray_ready = B8_FALSE;
    ctx->should_quit = B8_FALSE;

    pthread_mutex_init(&ctx->ready_mutex, NULL);
    pthread_cond_init(&ctx->ready_cond, NULL);

    g_ctx = ctx;
    return ctx;
}

nil_t raygui_ctx_destroy(raygui_ctx_t* ctx) {
    if (!ctx) return;

    if (ctx->ui_to_ray) raygui_queue_destroy(ctx->ui_to_ray);
    if (ctx->ray_to_ui) raygui_queue_destroy(ctx->ray_to_ui);

    pthread_mutex_destroy(&ctx->ready_mutex);
    pthread_cond_destroy(&ctx->ready_cond);

    free(ctx);
    g_ctx = NULL;
}

nil_t raygui_ctx_wait_ready(raygui_ctx_t* ctx) {
    pthread_mutex_lock(&ctx->ready_mutex);
    while (!ctx->ray_ready) {
        pthread_cond_wait(&ctx->ready_cond, &ctx->ready_mutex);
    }
    pthread_mutex_unlock(&ctx->ready_mutex);
}

nil_t raygui_ctx_signal_ready(raygui_ctx_t* ctx) {
    pthread_mutex_lock(&ctx->ready_mutex);
    ctx->ray_ready = B8_TRUE;
    pthread_cond_signal(&ctx->ready_cond);
    pthread_mutex_unlock(&ctx->ready_mutex);
}
```

**Step 3: Update Makefile**

Add `src/context.c` to SRC list.

**Step 4: Build and verify**

Run: `make clean && make`
Expected: Compiles successfully

**Step 5: Commit**

```bash
git add include/raygui/context.h src/context.c Makefile
git commit -m "feat: add context and threading synchronization"
```

---

## Task 6: Rayforce Thread

**Files:**
- Create: `src/rayforce_thread.c`
- Create: `include/raygui/rayforce_thread.h`

**Step 1: Create rayforce thread header**

```c
// include/raygui/rayforce_thread.h
#ifndef RAYGUI_RAYFORCE_THREAD_H
#define RAYGUI_RAYFORCE_THREAD_H

#include "context.h"

// Start rayforce thread
i32_t raygui_rayforce_start(raygui_ctx_t* ctx);

// Stop rayforce thread (sends quit message)
nil_t raygui_rayforce_stop(raygui_ctx_t* ctx);

// Register raygui types and functions with rayforce
nil_t raygui_register_types(nil_t);
nil_t raygui_register_fns(nil_t);

#endif // RAYGUI_RAYFORCE_THREAD_H
```

**Step 2: Implement rayforce thread**

```c
// src/rayforce_thread.c
#include "../include/raygui/rayforce_thread.h"
#include "../include/raygui/message.h"
#include "../include/raygui/widget.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations
static nil_t on_ui_message(raw_p data);
static raw_p rayforce_thread_main(raw_p arg);

// External type drop function for widget
static nil_t widget_ext_drop(raw_p ptr) {
    raygui_widget_destroy((raygui_widget_t*)ptr);
}

nil_t raygui_register_types(nil_t) {
    // Widget external type is created via external() function
    // The drop function is passed when creating the external object
}

nil_t raygui_register_fns(nil_t) {
    // TODO: Register widget and draw functions
    // This requires modifying rayforce's env to add custom functions
}

static nil_t on_ui_message(raw_p data) {
    raygui_ctx_t* ctx = (raygui_ctx_t*)data;
    raygui_ui_msg_t* msg;

    while ((msg = raygui_queue_pop(ctx->ui_to_ray)) != NULL) {
        switch (msg->type) {
            case RAYGUI_MSG_EVAL: {
                printf("[ray] Eval: %s\n", msg->expr);
                obj_p result = eval_str(msg->expr);

                // Format result
                obj_p fmt = obj_fmt(result, B8_TRUE);

                // Send result back to UI
                raygui_ray_msg_t* reply = malloc(sizeof(raygui_ray_msg_t));
                reply->type = RAYGUI_MSG_RESULT;
                reply->widget = msg->widget;
                reply->data = NULL;
                reply->text = strdup(AS_C8(fmt));

                drop_obj(fmt);
                drop_obj(result);

                raygui_queue_push(ctx->ray_to_ui, reply);
                // TODO: glfwPostEmptyEvent() to wake UI

                free(msg->expr);
                break;
            }

            case RAYGUI_MSG_DROP:
                if (msg->obj) drop_obj(msg->obj);
                break;

            case RAYGUI_MSG_QUIT:
                ctx->should_quit = B8_TRUE;
                poll_exit(runtime_get()->poll, 0);
                break;

            default:
                break;
        }
        free(msg);
    }
}

static raw_p rayforce_thread_main(raw_p arg) {
    raygui_ctx_t* ctx = (raygui_ctx_t*)arg;

    // Create runtime
    runtime_p runtime = runtime_create(ctx->argc, ctx->argv);
    if (!runtime) {
        fprintf(stderr, "Failed to create rayforce runtime\n");
        raygui_ctx_signal_ready(ctx);
        return NULL;
    }

    // Register types and functions
    raygui_register_types();
    raygui_register_fns();

    // Create waker for UI messages
    ctx->waker = poll_waker_create(runtime->poll, on_ui_message, ctx);

    // Signal ready
    raygui_ctx_signal_ready(ctx);
    printf("[ray] Rayforce thread ready\n");

    // Run poll loop
    runtime_run();

    // Cleanup
    if (ctx->waker) poll_waker_destroy(ctx->waker);
    runtime_destroy();

    printf("[ray] Rayforce thread exiting\n");
    return NULL;
}

i32_t raygui_rayforce_start(raygui_ctx_t* ctx) {
    return pthread_create(&ctx->ray_thread, NULL, rayforce_thread_main, ctx);
}

nil_t raygui_rayforce_stop(raygui_ctx_t* ctx) {
    // Send quit message
    raygui_ui_msg_t* msg = malloc(sizeof(raygui_ui_msg_t));
    msg->type = RAYGUI_MSG_QUIT;
    msg->expr = NULL;
    msg->obj = NULL;
    msg->widget = NULL;

    raygui_queue_push(ctx->ui_to_ray, msg);
    if (ctx->waker) poll_waker_wake(ctx->waker);

    // Wait for thread
    pthread_join(ctx->ray_thread, NULL);
}
```

**Step 3: Update Makefile**

Add `src/rayforce_thread.c` to SRC list. Add format.h include path.

**Step 4: Build and verify**

Run: `make clean && make`
Expected: Compiles successfully

**Step 5: Commit**

```bash
git add include/raygui/rayforce_thread.h src/rayforce_thread.c Makefile
git commit -m "feat: add rayforce thread with waker integration"
```

---

## Task 7: Update main.c with Thread Startup

**Files:**
- Modify: `src/main.c`
- Modify: `include/raygui/raygui.h`

**Step 1: Update raygui.h**

```c
// include/raygui/raygui.h
#ifndef RAYGUI_H
#define RAYGUI_H

#include "../../deps/rayforce/core/rayforce.h"
#include "context.h"
#include "rayforce_thread.h"
#include "queue.h"
#include "message.h"
#include "widget.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RAYGUI_VERSION_MAJOR 0
#define RAYGUI_VERSION_MINOR 1

i32_t raygui_init(i32_t argc, str_p argv[]);
i32_t raygui_run(nil_t);
nil_t raygui_destroy(nil_t);

// Send expression to rayforce for evaluation
nil_t raygui_eval(const char* expr);

#ifdef __cplusplus
}
#endif

#endif // RAYGUI_H
```

**Step 2: Update main.c**

```c
// src/main.c
#include "../include/raygui/raygui.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static raygui_ctx_t* ctx = NULL;

i32_t raygui_init(i32_t argc, str_p argv[]) {
    printf("raygui v%d.%d\n", RAYGUI_VERSION_MAJOR, RAYGUI_VERSION_MINOR);

    // Create context
    ctx = raygui_ctx_create(argc, argv);
    if (!ctx) {
        fprintf(stderr, "Failed to create context\n");
        return -1;
    }

    // Start rayforce thread
    if (raygui_rayforce_start(ctx) != 0) {
        fprintf(stderr, "Failed to start rayforce thread\n");
        raygui_ctx_destroy(ctx);
        return -1;
    }

    // Wait for rayforce to be ready
    raygui_ctx_wait_ready(ctx);

    return 0;
}

nil_t raygui_eval(const char* expr) {
    if (!ctx) return;

    raygui_ui_msg_t* msg = malloc(sizeof(raygui_ui_msg_t));
    msg->type = RAYGUI_MSG_EVAL;
    msg->expr = strdup(expr);
    msg->obj = NULL;
    msg->widget = NULL;

    raygui_queue_push(ctx->ui_to_ray, msg);
    if (ctx->waker) poll_waker_wake(ctx->waker);
}

i32_t raygui_run(nil_t) {
    // TODO: GLFW/ImGui main loop
    // For now, simple test loop

    printf("[ui] Sending test expression...\n");
    raygui_eval("(+ 1 2 3)");

    // Process responses
    sleep(1);

    raygui_ray_msg_t* reply;
    while ((reply = raygui_queue_pop(ctx->ray_to_ui)) != NULL) {
        if (reply->type == RAYGUI_MSG_RESULT) {
            printf("[ui] Result: %s\n", reply->text);
            free(reply->text);
        }
        free(reply);
    }

    return 0;
}

nil_t raygui_destroy(nil_t) {
    if (!ctx) return;

    raygui_rayforce_stop(ctx);
    raygui_ctx_destroy(ctx);
    ctx = NULL;
}

i32_t main(i32_t argc, str_p argv[]) {
    if (raygui_init(argc, argv) != 0) {
        return -1;
    }

    i32_t code = raygui_run();
    raygui_destroy();

    return code;
}
```

**Step 3: Build and test**

Run: `make clean && make && ./raygui`
Expected:
```
raygui v0.1
[ray] Rayforce thread ready
[ui] Sending test expression...
[ray] Eval: (+ 1 2 3)
[ui] Result: 6
[ray] Rayforce thread exiting
```

**Step 4: Commit**

```bash
git add include/raygui/raygui.h src/main.c
git commit -m "feat: integrate rayforce thread with main, test eval"
```

---

## Task 8: Add ImGui Dependencies

**Files:**
- Modify: `Makefile`
- Create: `deps/fetch_deps.sh`

**Step 1: Create dependency fetch script**

```bash
#!/bin/bash
# deps/fetch_deps.sh

cd "$(dirname "$0")"

# ImGui (docking branch)
if [ ! -d "imgui" ]; then
    git clone --depth 1 --branch docking https://github.com/ocornut/imgui.git
fi

# ImPlot
if [ ! -d "implot" ]; then
    git clone --depth 1 https://github.com/epezent/implot.git
fi

# GLFW (if not system-installed)
# if [ ! -d "glfw" ]; then
#     git clone --depth 1 https://github.com/glfw/glfw.git
# fi

echo "Dependencies fetched."
```

**Step 2: Run fetch script**

```bash
chmod +x deps/fetch_deps.sh
./deps/fetch_deps.sh
```

**Step 3: Update Makefile for ImGui**

```makefile
# Add to Makefile

IMGUI_DIR = deps/imgui
IMPLOT_DIR = deps/implot

IMGUI_SRC = $(IMGUI_DIR)/imgui.cpp \
            $(IMGUI_DIR)/imgui_demo.cpp \
            $(IMGUI_DIR)/imgui_draw.cpp \
            $(IMGUI_DIR)/imgui_tables.cpp \
            $(IMGUI_DIR)/imgui_widgets.cpp \
            $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp \
            $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp

IMPLOT_SRC = $(IMPLOT_DIR)/implot.cpp \
             $(IMPLOT_DIR)/implot_items.cpp

# Update INCLUDES
INCLUDES = -Iinclude -I$(RAYFORCE_DIR)/core -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends -I$(IMPLOT_DIR)
```

**Step 4: Commit**

```bash
git add deps/fetch_deps.sh Makefile
git commit -m "feat: add ImGui and ImPlot dependency setup"
```

---

## Remaining Tasks (Summary)

### Task 9: ImGui/GLFW Main Loop
- Initialize GLFW window
- Initialize ImGui with docking
- Main render loop with `glfwWaitEventsTimeout`

### Task 10: Widget Registry (UI Side)
- Track active widgets
- Render widgets in docked panels

### Task 11: Grid Widget Renderer
- ImGui table with `ImGuiListClipper`
- Zero-copy column access

### Task 12: REPL Widget
- Input buffer with history
- Output area with scroll

### Task 13: widget/draw Rayfall Functions
- Register as internal functions
- Create widget objects
- Send to UI

### Task 14: Post-Query Support
- Parse expression in Rayforce thread
- Apply before sending to UI

### Task 15: Chart Widget (ImPlot)
- Line/bar/scatter plots
- Zero-copy from float columns

### Task 16: Text Widget
- Simple value display

### Task 17: User Interaction → Post-Query
- Grid row selection
- Send filter expression

### Task 18: Docking Persistence
- Save/load dock layout

---

## Verification

After completing all tasks:

1. **Run the application:**
   ```bash
   ./raygui
   ```

2. **Test REPL:**
   - Type `(+ 1 2 3)` → see `6`
   - Type `(set t (table [a b] (list [1 2 3] [4 5 6])))` → see `table<...>`
   - Type `(set g (widget {type: 'grid name: "test"}))` → see grid panel appear
   - Type `(draw g t)` → see data in grid

3. **Test interaction:**
   - Click row in grid
   - Verify filter applies

4. **Test performance:**
   - Create table with 100k rows
   - Verify smooth scrolling via virtualization
