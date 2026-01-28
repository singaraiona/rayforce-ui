// include/rfui/rayforce_thread.h
#ifndef RFUI_RAYFORCE_THREAD_H
#define RFUI_RAYFORCE_THREAD_H

#include "context.h"

#ifdef __cplusplus
extern "C" {
#endif

// Thread entry point for Rayforce worker thread
// arg is rfui_ctx_t*
void* rfui_rayforce_thread(void* arg);

#ifdef __cplusplus
}
#endif

#endif // RFUI_RAYFORCE_THREAD_H
