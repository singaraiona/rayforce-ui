// src/ui.cpp
// ImGui/GLFW UI implementation for rayforce-ui

// Include C string functions before ImGui (ImGui uses memset, memcmp, strcmp)
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"

#include "../include/rfui/theme.h"
#include "../include/rfui/logo.h"

#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h>

// Make rayforce headers C++ compatible by redefining _Static_assert
// (rayforce uses C11 _Static_assert, C++ uses static_assert)
#define _Static_assert static_assert

// Include C headers for rayforce-ui
extern "C" {
#include "../include/rfui/ui.h"
#include "../include/rfui/context.h"
#include "../include/rfui/message.h"
#include "../include/rfui/queue.h"
#include "../include/rfui/widget_registry.h"
#include "../include/rfui/repl_renderer.h"
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
static char* g_ini_path = nullptr;  // ImGui layout persistence path

// Get config directory path, creating it if necessary
// Returns allocated string that caller must free, or nullptr on failure
static char* get_config_path(void) {
    const char* home = getenv("HOME");
    if (!home) return nullptr;

    // Use ~/.config/rfui/ as config directory
    const char* config_subdir = "/.config/rayforce-ui";
    const char* ini_filename = "/layout.ini";

    size_t dir_len = strlen(home) + strlen(config_subdir);
    size_t path_len = dir_len + strlen(ini_filename) + 1;

    // Create config directory if it doesn't exist
    char* dir_path = (char*)malloc(dir_len + 1);
    if (!dir_path) return nullptr;
    snprintf(dir_path, dir_len + 1, "%s%s", home, config_subdir);

    // Create ~/.config if needed
    char* config_dir = (char*)malloc(strlen(home) + strlen("/.config") + 1);
    if (config_dir) {
        snprintf(config_dir, strlen(home) + strlen("/.config") + 1, "%s/.config", home);
        mkdir(config_dir, 0755);  // Ignore error if exists
        free(config_dir);
    }

    // Create ~/.config/rayforce-ui if needed
    mkdir(dir_path, 0755);  // Ignore error if exists
    free(dir_path);

    // Return full path to ini file
    char* ini_path = (char*)malloc(path_len);
    if (!ini_path) return nullptr;
    snprintf(ini_path, path_len, "%s%s%s", home, config_subdir, ini_filename);

    return ini_path;
}

// External context (set by main.c)
extern rfui_ctx_t* g_ctx;

extern "C" {

i32_t rfui_ui_init(nil_t) {
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
    g_window = glfwCreateWindow(1280, 720, "Rayforce UI", nullptr, nullptr);
    if (g_window == nullptr) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(g_window);
    glfwSwapInterval(1); // Enable vsync

    // Get HiDPI scale factor
    float xscale, yscale;
    glfwGetWindowContentScale(g_window, &xscale, &yscale);
    float dpi_scale = (xscale > yscale) ? xscale : yscale;
    if (dpi_scale < 1.0f) dpi_scale = 1.0f;

    // Setup Dear ImGui and ImPlot contexts
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Docking

    // Set up persistent layout file (ImGui auto-saves on shutdown, auto-loads on startup)
    g_ini_path = get_config_path();
    if (g_ini_path) {
        io.IniFilename = g_ini_path;
    }
    // If get_config_path() fails, io.IniFilename remains "imgui.ini" (ImGui default)

    // Apply dashboard theme (replaces StyleColorsDark)
    rfui_theme_apply();

    // Load JetBrains Mono font at proper size for HiDPI
    float font_size = 16.0f * dpi_scale;
    io.Fonts->AddFontFromFileTTF("assets/fonts/JetBrainsMono-Regular.ttf", font_size);

    // Merge FontAwesome icons into the regular font
    static const ImWchar icon_ranges[] = { 0xf000, 0xf8ff, 0 };
    ImFontConfig icon_cfg;
    icon_cfg.MergeMode = true;
    icon_cfg.PixelSnapH = true;
    icon_cfg.GlyphMinAdvanceX = font_size;
    io.Fonts->AddFontFromFileTTF("assets/fonts/fa-solid-900.otf", font_size, &icon_cfg, icon_ranges);

    // Large font for text/label widgets (index 1)
    float large_font_size = 48.0f * dpi_scale;
    io.Fonts->AddFontFromFileTTF("assets/fonts/JetBrainsMono-Regular.ttf", large_font_size);

    // Scale style for HiDPI (but not fonts - they're already sized correctly)
    ImGui::GetStyle().ScaleAllSizes(dpi_scale);

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(g_window, true);
    ImGui_ImplOpenGL3_Init(g_glsl_version);

    // Load background logo and window icon (non-fatal if missing)
    rfui_logo_init("assets/images/logo.svg");
    rfui_icon_init("assets/images/icon.svg", g_window);

    // Initialize widget registry
    rfui_registry_init();

    g_initialized = true;
    return 0;
}

i32_t rfui_ui_run(nil_t) {
    if (!g_initialized || !g_window) {
        fprintf(stderr, "UI not initialized\n");
        return -1;
    }

    if (!g_ctx) {
        fprintf(stderr, "Context not set\n");
        return -1;
    }

    ImVec4 clear_color = ImVec4(0.051f, 0.067f, 0.090f, 1.0f);

    // Main loop
    while (!glfwWindowShouldClose(g_window) && !rfui_ctx_get_quit(g_ctx)) {
        // Poll events and use timeout to avoid busy-waiting
        // Note: We always poll first, then process messages
        glfwWaitEventsTimeout(0.016); // ~60fps timeout

        // Skip rendering if window is minimized
        if (glfwGetWindowAttrib(g_window, GLFW_ICONIFIED) != 0) {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        // Process messages from ray_to_ui queue (limited per frame)
        // Note: rfui_queue_pop returns NULL if queue is empty or NULL,
        // so we don't need a separate empty check (avoids TOCTOU race)
        int messages_processed = 0;
        rfui_ray_msg_t* msg;
        if (g_ctx->ray_to_ui != nullptr) {
            while (messages_processed < MAX_MESSAGES_PER_FRAME &&
                   (msg = (rfui_ray_msg_t*)rfui_queue_pop(g_ctx->ray_to_ui)) != nullptr) {
                // TODO: Process message based on type
                // For now: just free message resources
                switch (msg->type) {
                    case RFUI_MSG_WIDGET_CREATED:
                        // Register widget in UI
                        if (msg->widget) {
                            rfui_registry_add(msg->widget);
                        }
                        break;
                    case RFUI_MSG_DRAW:
                        // Update widget render_data and queue old data for drop
                        if (msg->widget) {
                            // For text widgets, store pre-formatted string in ui_state
                            if (msg->widget->type == RFUI_WIDGET_TEXT && msg->text) {
                                if (msg->widget->ui_state) {
                                    free(msg->widget->ui_state);
                                }
                                msg->widget->ui_state = msg->text;
                                msg->text = nullptr;
                            }
                            obj_p old_data = rfui_registry_update_data(msg->widget, msg->data);
                            // Queue old data for drop in Rayforce thread (if not NULL)
                            if (old_data) {
                                rfui_ui_msg_t* drop_msg = (rfui_ui_msg_t*)malloc(sizeof(rfui_ui_msg_t));
                                if (drop_msg) {
                                    drop_msg->type = RFUI_MSG_DROP;
                                    drop_msg->obj = old_data;
                                    drop_msg->widget = nullptr;
                                    drop_msg->expr = nullptr;
                                    if (!rfui_queue_push(g_ctx->ui_to_ray, drop_msg)) {
                                        // Queue full - leak rather than crash
                                        // (drop_obj requires Rayforce thread runtime)
                                        free(drop_msg);
                                    } else {
                                        poll_waker_p waker = rfui_ctx_get_waker(g_ctx);
                                        if (waker) poll_waker_wake(waker);
                                    }
                                } else {
                                    // Malloc failed - leak (drop_obj requires Rayforce thread)
                                    (void)old_data;
                                }
                            }
                        } else if (msg->data) {
                            // No widget - queue data for drop directly
                            rfui_ui_msg_t* drop_msg = (rfui_ui_msg_t*)malloc(sizeof(rfui_ui_msg_t));
                            if (drop_msg) {
                                drop_msg->type = RFUI_MSG_DROP;
                                drop_msg->obj = msg->data;
                                drop_msg->widget = nullptr;
                                drop_msg->expr = nullptr;
                                if (!rfui_queue_push(g_ctx->ui_to_ray, drop_msg)) {
                                    // Queue push failed - fall back to direct drop
                                    // Leak (drop_obj requires Rayforce thread)
                                        (void)msg->data;
                                    free(drop_msg);
                                } else {
                                    poll_waker_p waker = rfui_ctx_get_waker(g_ctx);
                                    if (waker) poll_waker_wake(waker);
                                }
                            } else {
                                // Malloc failed - leak (drop_obj requires Rayforce thread)
                                (void)msg->data;
                            }
                        }
                        break;
                    case RFUI_MSG_RESULT:
                        // Display result in REPL widget
                        if (msg->text) {
                            rfui_widget_t* repl = rfui_registry_find_by_type(RFUI_WIDGET_REPL);
                            if (repl) {
                                rfui_repl_add_result(repl, msg->text);
                            }
                        }
                        // Queue data for drop if present
                        if (msg->data) {
                            rfui_ui_msg_t* drop_msg = (rfui_ui_msg_t*)malloc(sizeof(rfui_ui_msg_t));
                            if (drop_msg) {
                                drop_msg->type = RFUI_MSG_DROP;
                                drop_msg->obj = msg->data;
                                drop_msg->widget = nullptr;
                                drop_msg->expr = nullptr;
                                if (!rfui_queue_push(g_ctx->ui_to_ray, drop_msg)) {
                                    // Queue push failed - fall back to direct drop
                                    // Leak (drop_obj requires Rayforce thread)
                                        (void)msg->data;
                                    free(drop_msg);
                                } else {
                                    poll_waker_p waker = rfui_ctx_get_waker(g_ctx);
                                    if (waker) poll_waker_wake(waker);
                                }
                            } else {
                                // Malloc failed - leak (drop_obj requires Rayforce thread)
                                (void)msg->data;
                            }
                        }
                        break;
                }

                if (msg->text) {
                    free(msg->text);
                }
                free(msg);
                messages_processed++;
            }
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Render background logo watermark (behind dockspace)
        rfui_logo_render();

        // Create dockspace — PassthruCentralNode makes empty areas transparent,
        // so the background logo watermark shows through
        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(),
                                     ImGuiDockNodeFlags_PassthruCentralNode);

        // Render all registered widgets
        rfui_registry_render();

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

nil_t rfui_ui_destroy(nil_t) {
    if (!g_initialized) {
        return;
    }

    // Destroy logo texture
    rfui_logo_destroy();

    // Destroy widget registry (frees all widgets)
    rfui_registry_destroy();

    // Cleanup ImGui and ImPlot
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    // Free ini path (after DestroyContext, which saves the layout)
    if (g_ini_path) {
        free(g_ini_path);
        g_ini_path = nullptr;
    }

    // Cleanup GLFW
    if (g_window) {
        glfwDestroyWindow(g_window);
        g_window = nullptr;
    }
    glfwTerminate();

    g_initialized = false;
}

b8_t rfui_ui_should_run(nil_t) {
    if (!g_initialized || !g_window) {
        return B8_FALSE;
    }
    return glfwWindowShouldClose(g_window) ? B8_FALSE : B8_TRUE;
}

nil_t rfui_ui_wake(nil_t) {
    glfwPostEmptyEvent();
}

} // extern "C"
