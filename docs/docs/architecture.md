# Architecture

## Threading Model

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

## Zero-Copy Data Flow

1. Rayforce evaluates query → produces `obj_p` (table/vector)
2. Passes raw `obj_p` to UI via queue (**no serialization**)
3. UI renders directly from columnar buffers
4. UI signals done → returns `obj_p` via queue
5. Rayforce calls `drop_obj` (refcount--)

**No copies, no marshaling** — pointer exchange only. Objects stay alive via refcount until UI releases.

## Communication

| Direction | Payload | Wake Mechanism |
|-----------|---------|----------------|
| UI → Rayforce | Expression string | `poll_waker_wake()` |
| Rayforce → UI | `obj_p` | `glfwPostEmptyEvent()` |
| UI → Rayforce | `obj_p` to drop | (batched) |
