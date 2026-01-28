// include/rfui/logo.h
// Background logo watermark

#ifndef RFUI_LOGO_H
#define RFUI_LOGO_H

#ifdef __cplusplus
extern "C" {
#endif

// Load logo SVG and create OpenGL texture. Returns 0 on success.
int rfui_logo_init(const char* svg_path);

// Load icon SVG and set as GLFW window icon. Returns 0 on success.
int rfui_icon_init(const char* svg_path, void* glfw_window);

// Render logo as centered background watermark (call before dockspace).
void rfui_logo_render(void);

// Free logo texture.
void rfui_logo_destroy(void);

// Get logo texture ID (cast to ImTextureID). Returns 0 if not loaded.
unsigned int rfui_logo_texture(void);

// Get logo dimensions.
void rfui_logo_size(int* w, int* h);

#ifdef __cplusplus
}
#endif

#endif // RFUI_LOGO_H
