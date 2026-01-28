// include/raygui/theme.h
// Dashboard theme for raygui

#ifndef RAYGUI_THEME_H
#define RAYGUI_THEME_H

#ifdef __cplusplus
extern "C" {
#endif

// Apply the raygui dashboard theme to ImGui and ImPlot.
// Must be called after ImGui::CreateContext() and ImPlot::CreateContext().
void raygui_theme_apply(void);

#ifdef __cplusplus
}
#endif

#endif // RAYGUI_THEME_H
