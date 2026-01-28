// src/ui.cpp
// ImGui/GLFW UI implementation for raygui

// Include C string functions before ImGui (ImGui uses memset, memcmp, strcmp)
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h>

// Make rayforce headers C++ compatible by redefining _Static_assert
// (rayforce uses C11 _Static_assert, C++ uses static_assert)
#define _Static_assert static_assert

// Include C headers for raygui
extern "C" {
#include "../include/raygui/ui.h"
#include "../include/raygui/context.h"
#include "../include/raygui/message.h"
#include "../include/raygui/queue.h"
}

// Maximum messages to process per frame to avoid blocking rendering
#define MAX_MESSAGES_PER_FRAME 64

// GLFW error callback
static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

// Global state
static GLFWwindow* g_window = nullptr;
static const char* g_glsl_version = nullptr;
static bool g_initialized = false;

// External context (set by main.c)
extern raygui_ctx_t* g_ctx;

extern "C" {

i32_t raygui_ui_init(nil_t) {
    if (g_initialized) {
        fprintf(stderr, "UI already initialized\n");
        return -1;
    }

    // Setup GLFW error callback
    glfwSetErrorCallback(glfw_error_callback);

    // Initialize GLFW
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100 (WebGL 1.0)
    g_glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    g_glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
    // GL 3.0 + GLSL 130
    g_glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

    // Create window with graphics context
    g_window = glfwCreateWindow(1280, 720, "raygui", nullptr, nullptr);
    if (g_window == nullptr) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(g_window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Docking

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(g_window, true);
    ImGui_ImplOpenGL3_Init(g_glsl_version);

    g_initialized = true;
    return 0;
}

i32_t raygui_ui_run(nil_t) {
    if (!g_initialized || !g_window) {
        fprintf(stderr, "UI not initialized\n");
        return -1;
    }

    if (!g_ctx) {
        fprintf(stderr, "Context not set\n");
        return -1;
    }

    ImVec4 clear_color = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);

    // Main loop
    while (!glfwWindowShouldClose(g_window) && !raygui_ctx_get_quit(g_ctx)) {
        // Check if there's pending work in the queue
        bool has_pending_work = !raygui_queue_empty(g_ctx->ray_to_ui);

        // Use wait with timeout if no pending work, otherwise poll
        if (has_pending_work) {
            glfwPollEvents();
        } else {
            glfwWaitEventsTimeout(0.1); // 100ms timeout
        }

        // Skip rendering if window is minimized
        if (glfwGetWindowAttrib(g_window, GLFW_ICONIFIED) != 0) {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        // Process messages from ray_to_ui queue (limited per frame)
        int messages_processed = 0;
        raygui_ray_msg_t* msg;
        while (messages_processed < MAX_MESSAGES_PER_FRAME &&
               (msg = (raygui_ray_msg_t*)raygui_queue_pop(g_ctx->ray_to_ui)) != nullptr) {
            // TODO: Process message based on type
            // For now: just free message resources
            switch (msg->type) {
                case RAYGUI_MSG_WIDGET_CREATED:
                    // TODO: Register widget in UI
                    break;
                case RAYGUI_MSG_DRAW:
                    // TODO: Update widget data
                    break;
                case RAYGUI_MSG_RESULT:
                    // TODO: Display in REPL widget
                    break;
            }

            if (msg->text) {
                free(msg->text);
            }
            free(msg);
            messages_processed++;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Create dockspace over the entire viewport
        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

        // TODO: Render widgets (placeholder for now)
        // This will be implemented when we add the widget registry

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(g_window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w,
                     clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(g_window);
    }

    return 0;
}

nil_t raygui_ui_destroy(nil_t) {
    if (!g_initialized) {
        return;
    }

    // Cleanup ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // Cleanup GLFW
    if (g_window) {
        glfwDestroyWindow(g_window);
        g_window = nullptr;
    }
    glfwTerminate();

    g_initialized = false;
}

b8_t raygui_ui_should_run(nil_t) {
    if (!g_initialized || !g_window) {
        return B8_FALSE;
    }
    return glfwWindowShouldClose(g_window) ? B8_FALSE : B8_TRUE;
}

nil_t raygui_ui_wake(nil_t) {
    glfwPostEmptyEvent();
}

} // extern "C"
