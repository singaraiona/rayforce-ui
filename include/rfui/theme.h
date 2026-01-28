// include/rfui/theme.h
// Dashboard theme for rayforce-ui

#ifndef RFUI_THEME_H
#define RFUI_THEME_H

#ifdef __cplusplus
extern "C" {
#endif

// Apply the rayforce-ui dashboard theme to ImGui and ImPlot.
// Must be called after ImGui::CreateContext() and ImPlot::CreateContext().
void rfui_theme_apply(void);

#ifdef __cplusplus
}
#endif

#endif // RFUI_THEME_H
