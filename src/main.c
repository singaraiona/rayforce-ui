// src/main.c
#include "../include/raygui/raygui.h"
#include <stdio.h>

// Stub implementations - to be filled in later tasks
i32_t raygui_init(i32_t argc, str_p argv[]) {
    (void)argc;
    (void)argv;
    return 0;
}

i32_t raygui_run(nil_t) {
    return 0;
}

nil_t raygui_destroy(nil_t) {
}

i32_t main(i32_t argc, str_p argv[]) {
    printf("raygui v%d.%d\n", RAYGUI_VERSION_MAJOR, RAYGUI_VERSION_MINOR);

    if (raygui_init(argc, argv) != 0) {
        return -1;
    }

    i32_t code = raygui_run();
    raygui_destroy();

    return code;
}
