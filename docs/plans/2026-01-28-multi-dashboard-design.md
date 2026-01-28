# Multi-Dashboard Design

## Overview

Support multiple OS-level windows for multi-monitor trading setups. Each dashboard is a GLFW window with its own dockspace. The main window (id 0) is the control window containing the REPL. Additional dashboards are display-only widget surfaces.

Everything is Rayfall-driven — no UI-based window creation.

## Rayfall API

```clj
;; Create dashboard (returns dashboard obj_p)
(set prices (dashboard {name: "Prices"}))
(set risk   (dashboard {name: "Risk"}))

;; Widgets target a dashboard
(set g1 (widget {type: 'grid name: "fills" dashboard: prices}))
(draw g1 fills-data)

;; Default dashboard is the main window
(set g2 (widget {type: 'grid name: "log"}))

;; Destroy dashboard — del drops refcount, triggers window close
(del prices)
```

No `monitor:` parameter. Users arrange windows manually. ImGui persists layout per window via ini file.

## Data Structures

### New: `rfui_dashboard_t`

```c
#define RFUI_MAX_DASHBOARDS 8

typedef struct rfui_dashboard_t {
    u32_t id;              // 0 = main window
    char* name;            // window title
    GLFWwindow* window;    // GLFW window, shared GL context with main
    b8_t is_open;          // false when destroyed or user-closed
} rfui_dashboard_t;
```

### Modified: `rfui_widget_t`

```c
typedef struct rfui_widget_t {
    // ... existing fields ...
    rfui_dashboard_t* dashboard;  // NULL = main window
} rfui_widget_t;
```

### Modified: `rfui_ctx_t`

```c
typedef struct rfui_ctx_t {
    // ... existing fields ...
    rfui_dashboard_t* dashboards[RFUI_MAX_DASHBOARDS];  // [0] = main
    u32_t dashboard_count;
} rfui_ctx_t;
```

## Message Protocol

Two new message types:

| Message | Direction | Payload | Purpose |
|---------|-----------|---------|---------|
| `RFUI_MSG_DASHBOARD_CREATED` | Rayforce -> UI | `rfui_dashboard_t*` | Create GLFW window |
| `RFUI_MSG_DASHBOARD_CLOSE` | Rayforce -> UI | `rfui_dashboard_t*` | Destroy GLFW window |

Existing `RFUI_MSG_WIDGET_CREATED` unchanged — widget carries `dashboard` pointer, UI reads it during render to select dockspace.

## Lifecycle

### Dashboard creation: `(dashboard {name: "Prices"})`

1. Rayforce thread allocates `rfui_dashboard_t`, sets name
2. Pushes `RFUI_MSG_DASHBOARD_CREATED` to `ray_to_ui` queue
3. Calls `glfwPostEmptyEvent()` to wake UI thread
4. UI thread creates GLFW window with shared GL context: `glfwCreateWindow(w, h, name, NULL, main_window)`
5. Stores window handle in dashboard struct
6. Adds dashboard to `ctx->dashboards[]`
7. Returns `obj_p` wrapping the dashboard to Rayfall

### Widget targeting: `(widget {type: 'grid dashboard: d})`

1. Rayforce resolves `d` to `rfui_dashboard_t*`
2. Sets `widget->dashboard = d`
3. Pushes `RFUI_MSG_WIDGET_CREATED` as before
4. UI render loop checks `widget->dashboard` to select dockspace

### Dashboard destruction: `(del d)`

1. `obj_p` refcount reaches zero
2. Registered `dashboard_drop` function runs
3. Sends `RFUI_MSG_DASHBOARD_CLOSE` to UI thread
4. UI thread calls `glfwDestroyWindow(dashboard->window)`
5. Marks all widgets with this dashboard as `is_open = false`
6. Removes from `ctx->dashboards[]`

## Render Loop

Current single-window loop becomes multi-window:

```c
for (u32_t i = 0; i < ctx->dashboard_count; i++) {
    rfui_dashboard_t* db = ctx->dashboards[i];
    if (!db->is_open) continue;

    glfwMakeContextCurrent(db->window);

    // Size viewport to this window
    int w, h;
    glfwGetFramebufferSize(db->window, &w, &h);
    glViewport(0, 0, w, h);

    // ImGui frame for this window
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Main window gets logo + REPL
    if (i == 0) {
        rfui_logo_render();
    }

    // Dockspace with unique ID per dashboard
    ImGuiID dockspace_id = ImGui::GetID(db->name);
    ImGui::DockSpaceOverViewport(dockspace_id,
        ImGui::GetMainViewport(),
        ImGuiDockNodeFlags_PassthruCentralNode);

    // Render widgets belonging to this dashboard
    rfui_registry_render_for(db);

    ImGui::Render();
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(db->window);
}
```

Event polling stays unified — `glfwPollEvents()` handles all windows.

## Performance

No concerns for this use case:
- Max 8 windows, GL context switch is microseconds
- Event-driven rendering (idle when no data), not spinning
- Zero-copy data path unchanged — rendering reads `obj_p` column buffers directly
- ImGui 2D rendering is lightweight across multiple windows
- GPU is idle in trading dashboards — plenty of headroom

## Rayforce Integration

Register dashboard as an external type:

```c
ray_register_ext("dashboard", dashboard_drop, dashboard_format);
ray_register_fn("dashboard", 1, fn_dashboard);
```

`dashboard_format` prints `"dashboard<Prices>"` for REPL display.

## File Changes

### New files
- `include/rfui/dashboard.h` — struct, constants
- `src/dashboard.c` — creation, drop, registry

### Modified files
- `include/rfui/message.h` — `RFUI_MSG_DASHBOARD_CREATED`, `RFUI_MSG_DASHBOARD_CLOSE`
- `include/rfui/widget.h` — add `rfui_dashboard_t* dashboard` field
- `include/rfui/context.h` — dashboard array in context
- `src/rayforce_thread.c` — register `dashboard` fn and ext type
- `src/ui.cpp` — multi-window render loop, dashboard message handling
- `src/widget_registry.cpp` — `rfui_registry_render_for(dashboard)` filter

### Unchanged
- Queue implementation
- Grid, chart, text, REPL renderers
- Theme, logo, fonts
