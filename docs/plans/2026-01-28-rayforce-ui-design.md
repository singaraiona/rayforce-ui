# rayforce-ui Design Document

**Date:** 2026-01-28
**Status:** Draft

## Overview

rayforce-ui is a GUI framework for RayforceDB that extends the Rayforce runtime with native widget functions. Widgets are first-class Rayforce objects defined and controlled via Rayfall expressions. Uses Dear ImGui for rendering.

### Goals

- Zero-copy data rendering from Rayforce columnar buffers
- REPL-centric interaction model (live coding environment)
- Dockable widget panels with persistent layouts
- Bidirectional communication via Rayforce IO poll

## Architecture

### Threading Model

```
Main Thread (UI)                    Worker Thread (Rayforce)
┌─────────────────────┐            ┌─────────────────────────┐
│ GLFW + ImGui        │            │ Rayforce Runtime        │
│ - Render loop       │◄──obj_p───│ - Thread-local heap     │
│ - Zero-copy render  │            │ - Eval expressions      │
│ - User interaction  │───string──►│ - Widget objects        │
│ - Returns obj_p     │───obj_p───►│ - drop_obj cleanup      │
└─────────────────────┘            └─────────────────────────┘
         ▲                                    ▲
         └──────────── IO Poll ───────────────┘
```

### Zero-Copy Data Flow

1. Rayforce evaluates query → produces `obj_p` (table/vector)
2. Passes raw `obj_p` to UI via queue (no serialization)
3. UI renders directly from columnar buffers
4. UI signals done → returns `obj_p` via queue
5. Rayforce calls `drop_obj` (refcount--)

**No copies, no marshaling** - pointer exchange only.

### Communication

| Direction | Payload | Mechanism |
|-----------|---------|-----------|
| UI → Rayforce | Expression string | Queue + `poll_waker_wake()` |
| Rayforce → UI | `obj_p` | Queue + `glfwPostEmptyEvent()` |
| UI → Rayforce | `obj_p` to drop | Queue |

### Message Types

**UI → Rayforce:**
- `MSG_EVAL` - expression string to evaluate
- `MSG_SET_POST_QUERY` - widget + query expression string
- `MSG_DROP` - obj_p done rendering
- `MSG_QUIT` - shutdown signal

**Rayforce → UI:**
- `MSG_WIDGET_CREATED` - new widget panel
- `MSG_DRAW` - widget + data obj_p
- `MSG_RESULT` - formatted result string for REPL

## Widget Model

### Rayfall API

```clj
;; Create widget (opens panel immediately)
(set grid1 (widget {type: 'grid name: "trades"}))

;; Push data to widget (replaces previous)
(draw grid1 (select {from: trades where: (> price 100)}))

;; Widget with interaction callback
(set grid1 (widget {type: 'grid name: "symbols"
                    on-select: (fn [row] (draw details row))}))
```

### Widget Lifecycle

1. `(widget {...})` - Creates widget object, opens empty docked panel
2. `(draw widget data)` - Sends data for rendering, replaces previous
3. User interaction → UI sends expression string to Rayforce
4. Rayforce parses, sets `widget->post_query` (obj_p)
5. Next draw applies `post_query` to data before rendering

### Post-Query

Widget struct has `post_query` field for persistent transformations:

```clj
;; Filter
"(select {from: data where: (== sym 'AAPL)})"

;; Aggregation
"(select {from: data by: sym total: (sum size)})"

;; Sort
"(xdesc ['price] data)"
```

UI sends expression string, Rayforce parses and allocates obj_p.

### Widget Structure

```c
typedef struct rfui_widget_t {
    rfui_widget_type_t type;  // GRID, CHART, TEXT, REPL
    char* name;                  // Panel title
    obj_p data;                  // Base data from draw()
    obj_p post_query;            // Expression applied before render
    obj_p on_select;             // Callback function

    // UI state (UI thread only)
    bool is_open;
    ImGuiID dock_id;
    void* ui_state;              // Type-specific UI state
    obj_p render_data;           // Current data for rendering
} rfui_widget_t;
```

## V1 Widget Types

| Type | Purpose | Key Features |
|------|---------|--------------|
| Grid | Table display | Row selection, column sorting, ImGuiListClipper |
| Chart | Data visualization | Line/bar/scatter via ImPlot |
| Text | Value display | Simple formatted output |
| REPL | Core interaction | Rayfall input, result output, history |

## Startup Sequence

```c
int main(int argc, char* argv[]) {
    // 1. Create queues
    ui_to_ray_queue = queue_create(QUEUE_SIZE);
    ray_to_ui_queue = queue_create(QUEUE_SIZE);

    // 2. Start Rayforce thread
    rfui_ctx_t ctx = { .argc = argc, .argv = argv };
    pthread_create(&ray_thread, NULL, rayforce_thread, &ctx);

    // 3. Wait for runtime ready
    wait_for_ready(&ctx);

    // 4. Initialize GLFW + ImGui (main thread)
    glfwInit();
    GLFWwindow* window = glfwCreateWindow(...);
    imgui_init(window);

    // 5. Create REPL widget
    rfui_eval("(set *repl* (widget {type: 'repl name: \"REPL\"}))");

    // 6. Main loop
    rfui_main_loop(window);

    // 7. Cleanup
    rfui_quit();
    pthread_join(ray_thread, NULL);
}

void* rayforce_thread(void* arg) {
    rfui_ctx_t* ctx = arg;

    // 1. Create runtime
    runtime_p runtime = runtime_create(ctx->argc, ctx->argv);

    // 2. Register rayforce-ui extensions
    rfui_register_types();   // widget ext type
    rfui_register_fns();     // widget, draw functions

    // 3. Create waker for UI messages
    ctx->waker = poll_waker_create(runtime->poll, on_ui_message, ctx);

    // 4. Signal ready
    signal_ready(ctx);

    // 5. Run poll loop
    runtime_run();

    // 6. Cleanup
    runtime_destroy();
    return NULL;
}
```

## UI Main Loop

```c
void rfui_main_loop(GLFWwindow* window) {
    while (!glfwWindowShouldClose(window)) {
        // Event handling with idle sleep
        if (has_pending_work()) {
            glfwPollEvents();
        } else {
            glfwWaitEventsTimeout(0.1);
        }

        // Process messages (limited per frame)
        ui_process_queue(MAX_MESSAGES_PER_FRAME);

        // Render
        imgui_new_frame();
        ui_render_widgets();
        imgui_render();

        // Return consumed obj_p for cleanup
        ui_flush_drop_queue();
    }
}

bool has_pending_work() {
    return !queue_empty(ray_to_ui_queue)
        || imgui_wants_input()
        || any_widget_animating();
}
```

## Grid Renderer (Zero-Copy)

```c
void render_grid(rfui_widget_t* w) {
    if (!w->render_data) return;

    obj_p table = w->render_data;
    obj_p cols = table_keys(table);
    obj_p vals = table_vals(table);
    i64_t ncols = vec_len(cols);
    i64_t nrows = ncols > 0 ? vec_len(vec_at(vals, 0)) : 0;

    if (ImGui::BeginTable(w->name, ncols, TABLE_FLAGS)) {
        // Headers
        for (i64_t c = 0; c < ncols; c++) {
            ImGui::TableSetupColumn(AS_SYM(vec_at(cols, c)));
        }
        ImGui::TableHeadersRow();

        // Virtualized rows (only visible rendered)
        ImGuiListClipper clipper;
        clipper.Begin(nrows);
        while (clipper.Step()) {
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
                ImGui::TableNextRow();
                for (i64_t c = 0; c < ncols; c++) {
                    ImGui::TableNextColumn();
                    obj_p col = vec_at(vals, c);
                    render_cell(col, row);  // Direct buffer access
                }
            }
        }
        ImGui::EndTable();
    }
}
```

## User Interaction Flow

```
User clicks row in grid
        │
        ▼
UI Thread:
  1. Detect selection
  2. Build expression: "(select {from: data where: (== sym 'AAPL)})"
  3. Send MSG_SET_POST_QUERY to queue
  4. poll_waker_wake(ray_waker)
        │
        ▼
Rayforce Thread:
  1. Waker callback fires
  2. Parse expression → obj_p
  3. Set widget->post_query
  4. Eval with current data
  5. Send MSG_DRAW with result
  6. glfwPostEmptyEvent()
        │
        ▼
UI Thread (next frame):
  1. Process MSG_DRAW
  2. Update render_data
  3. Render filtered grid
  4. Queue old obj_p for drop
```

## Rayforce Extensions

### Type Registration

```c
void rfui_register_types() {
    ray_register_ext("widget", widget_drop, widget_format);
}

char* widget_format(obj_p w) {
    rfui_widget_t* widget = AS_EXT(w);
    return format_str("widget<%s:\"%s\">",
        widget_type_name(widget->type),
        widget->name);
}
```

### Function Registration

```c
void rfui_register_fns() {
    ray_register_fn("widget", 1, fn_widget);
    ray_register_fn("draw", 2, fn_draw);
}
```

## Performance Considerations

- **Zero-copy**: UI reads directly from obj_p column buffers
- **Virtualization**: ImGuiListClipper for large tables
- **Burst limiting**: Max messages per frame prevents stalls
- **Idle sleep**: glfwWaitEventsTimeout when no activity
- **Refcounting**: Objects stay alive during render, no races

## File Structure

```
rfui/
├── include/
│   └── rfui/
│       ├── rayforce-ui.h        # Public API
│       ├── widget.h        # Widget types and struct
│       └── queue.h         # Thread-safe queues
├── src/
│   ├── main.c              # Entry point, startup
│   ├── widget.c            # fn_widget, fn_draw
│   ├── render.c            # UI rendering
│   ├── grid.c              # Grid widget
│   ├── chart.c             # Chart widget (ImPlot)
│   ├── text.c              # Text widget
│   ├── repl.c              # REPL widget
│   └── queue.c             # Queue implementation
├── deps/
│   ├── rayforce/           # Rayforce submodule
│   ├── imgui/              # Dear ImGui
│   ├── implot/             # ImPlot
│   └── glfw/               # GLFW
└── docs/
    └── plans/
```

## Next Steps

1. Set up build system (CMake or Makefile)
2. Implement queue and waker integration
3. Implement widget/draw functions
4. Implement REPL widget
5. Implement Grid widget
6. Implement Chart widget
7. Add docking persistence
