# Getting Started

**rayforce-ui** is a GUI framework for RayforceDB that extends the Rayforce runtime with native widget functions. Widgets are first-class Rayforce objects defined and controlled via Rayfall expressions.

## Built On

- **Rayforce** — Columnar database engine with pure C core, thread-local heaps, and IO poll
- **Rayfall** — LISP-like query language extended with widget/draw functions
- **Dear ImGui** — Immediate-mode GUI with docking support
- **ImPlot** — Real-time chart rendering

## Prerequisites

- C compiler (GCC or Clang)
- GLFW 3.x
- Rayforce runtime library

## Startup Pattern

```c
// Follow Rayforce's runtime pattern
runtime_p runtime = runtime_create(argc, argv);
rfui_register_types();  // widget ext type with format fn
rfui_register_fns();    // widget, draw
poll_waker_create(runtime->poll, on_ui_message, ctx);
runtime_run();  // blocks until poll_exit
runtime_destroy();
```

## External Type Registration

```c
ray_register_ext("widget", widget_drop, widget_format);
ray_register_fn("widget", 1, fn_widget);
ray_register_fn("draw", 2, fn_draw);
```

## Performance Guidelines

- Zero-copy rendering: read directly from `obj_p` column buffers
- Use `ImGuiListClipper` for virtualized table rendering
- Limit messages per frame to prevent stalls
- Use `glfwWaitEventsTimeout()` when idle to save CPU
- Refcount keeps data alive during render — no races

## Naming Convention

Use `ray` prefix (not `rf`):

- `ray_register_ext`, `ray_register_fn`
- `rfui_widget_t`, `rfui_ctx_t`
