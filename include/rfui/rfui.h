// include/rfui/rayforce-ui.h
#ifndef RFUI_H
#define RFUI_H

#include "../../deps/rayforce/core/rayforce.h"
#include "../../deps/rayforce/core/runtime.h"
#include "../../deps/rayforce/core/poll.h"

#ifdef __cplusplus
extern "C" {
#endif

// Version
#define RFUI_VERSION_MAJOR 0
#define RFUI_VERSION_MINOR 1

// Initialize rayforce-ui (call from main thread before starting rayforce thread)
i32_t rfui_init(i32_t argc, str_p argv[]);

// Run main loop (blocks until quit)
i32_t rfui_run(nil_t);

// Cleanup
nil_t rfui_destroy(nil_t);

// Send expression to Rayforce thread for evaluation
i32_t rfui_eval(const char* expr);

#ifdef __cplusplus
}
#endif

#endif // RFUI_H
