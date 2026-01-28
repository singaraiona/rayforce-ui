// include/raygui/logo.h
// Background logo watermark

#ifndef RAYGUI_LOGO_H
#define RAYGUI_LOGO_H

#ifdef __cplusplus
extern "C" {
#endif

// Load logo SVG and create OpenGL texture. Returns 0 on success.
int raygui_logo_init(const char* svg_path);

// Render logo as centered background watermark (call before dockspace).
void raygui_logo_render(void);

// Free logo texture.
void raygui_logo_destroy(void);

#ifdef __cplusplus
}
#endif

#endif // RAYGUI_LOGO_H
