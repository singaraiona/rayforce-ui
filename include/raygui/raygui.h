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
