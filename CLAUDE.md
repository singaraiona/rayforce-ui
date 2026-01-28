# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**rayforce-ui** is a GUI framework for RayforceDB that extends the Rayforce runtime with native widget functions. Widgets are first-class Rayforce objects defined and controlled via Rayfall expressions.

Built on:
- **Rayforce**: Columnar database engine with pure C core, thread-local heaps, and IO poll
- **Rayfall**: LISP-like query language extended with widget/draw functions
- **Dear ImGui**: Immediate-mode GUI with docking support
- **ImPlot**: Real-time chart rendering

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
         └──────────── Queues ────────────────┘
```

### Zero-Copy Data Flow

1. Rayforce evaluates query → produces `obj_p` (table/vector)
2. Passes raw `obj_p` to UI via queue (**no serialization**)
3. UI renders directly from columnar buffers
4. UI signals done → returns `obj_p` via queue
5. Rayforce calls `drop_obj` (refcount--)

**No copies, no marshaling** - pointer exchange only. Objects stay alive via refcount until UI releases.

### Communication

| Direction | Payload | Wake Mechanism |
|-----------|---------|----------------|
| UI → Rayforce | Expression string | `poll_waker_wake()` |
| Rayforce → UI | `obj_p` | `glfwPostEmptyEvent()` |
| UI → Rayforce | `obj_p` to drop | (batched) |

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
4. Rayforce parses, allocates `obj_p`, sets `widget->post_query`
5. Next draw applies `post_query` to data before rendering

### Post-Query

Any Rayfall expression applied to data before rendering:

```clj
"(select {from: data where: (== sym 'AAPL)})"  ;; Filter
"(select {from: data by: sym total: (sum size)})"  ;; Aggregate
"(xdesc ['price] data)"  ;; Sort
```

All `obj_p` allocation happens in Rayforce thread (thread-local heaps).

## V1 Widget Types

| Type | Purpose | Key Features |
|------|---------|--------------|
| Grid | Table display | Row selection, column sorting, virtualized |
| Chart | Data visualization | Line/bar/scatter via ImPlot |
| Text | Value display | Simple formatted output |
| REPL | Core interaction | Rayfall input, result output, history |

## Rayforce Integration

### Startup Pattern

```c
// Follow Rayforce's runtime pattern
runtime_p runtime = runtime_create(argc, argv);
rfui_register_types();  // widget ext type with format fn
rfui_register_fns();    // widget, draw
poll_waker_create(runtime->poll, on_ui_message, ctx);
runtime_run();  // blocks until poll_exit
runtime_destroy();
```

### External Type Registration

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
- Refcount keeps data alive during render - no races

## Naming Convention

Use `ray` prefix (not `rf`):
- `ray_register_ext`, `ray_register_fn`
- `rfui_widget_t`, `rfui_ctx_t`

## Specialized Agents

- **rayforce-expert**: Rayfall queries, zero-copy data flows, Rayforce C API
- **imgui-expert**: Dear ImGui widgets, rendering optimization, docking
- **ide-creator**: Code editor components, syntax highlighting

## Rayfall Skill

Invoke `/rayfall` for complete language reference.

## Design Document

See `docs/plans/2026-01-28-rayforce-ui-design.md` for full architecture.
