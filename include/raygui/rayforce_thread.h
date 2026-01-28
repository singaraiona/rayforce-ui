// include/raygui/rayforce_thread.h
#ifndef RAYGUI_RAYFORCE_THREAD_H
#define RAYGUI_RAYFORCE_THREAD_H

#include "context.h"

#ifdef __cplusplus
extern "C" {
#endif

// Thread entry point for Rayforce worker thread
// arg is raygui_ctx_t*
void* raygui_rayforce_thread(void* arg);

#ifdef __cplusplus
}
#endif

#endif // RAYGUI_RAYFORCE_THREAD_H
