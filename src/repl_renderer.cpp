// src/repl_renderer.cpp
// Terminal-style REPL renderer — renders directly into the main window

#include <stdio.h>
#include <string.h>
#include <vector>
#include <string>

#include "imgui.h"
#include "../include/rfui/icons.h"

// Scrollback limits
static const int MAX_HISTORY_SIZE = 1000;
static const int MAX_OUTPUT_LINES = 10000;

#define _Static_assert static_assert

extern "C" {
#include "../include/rfui/repl_renderer.h"
#include "../include/rfui/rfui.h"
}

// Line type for terminal display
enum LineType {
    LINE_INPUT,   // "> expression" - user input
    LINE_RESULT,  // result of evaluation
    LINE_ERROR    // error message
};

struct terminal_line_t {
    std::string text;
    LineType type;
};

// REPL state
struct repl_state_t {
    char input_buf[4096];
    std::vector<std::string> history;      // Command history for up/down
    std::vector<terminal_line_t> lines;    // Terminal output lines
    int history_pos;
    bool scroll_to_bottom;
    std::string saved_input;
};

// Module-level REPL state (singleton — one REPL per application)
static repl_state_t* g_repl = nullptr;

// Standard ANSI 8-color palette
static const ImVec4 ansi_colors[8] = {
    ImVec4(0.0f,   0.0f,   0.0f,   1.0f),  // 0 black
    ImVec4(0.804f, 0.141f, 0.114f, 1.0f),  // 1 red
    ImVec4(0.247f, 0.725f, 0.314f, 1.0f),  // 2 green
    ImVec4(0.824f, 0.600f, 0.133f, 1.0f),  // 3 yellow
    ImVec4(0.345f, 0.651f, 1.000f, 1.0f),  // 4 blue
    ImVec4(0.737f, 0.549f, 1.000f, 1.0f),  // 5 magenta
    ImVec4(0.224f, 0.824f, 0.753f, 1.0f),  // 6 cyan
    ImVec4(0.902f, 0.929f, 0.953f, 1.0f),  // 7 white
};

// Bright ANSI colors (90-97)
static const ImVec4 ansi_bright[8] = {
    ImVec4(0.545f, 0.580f, 0.620f, 1.0f),  // 0 bright black (gray)
    ImVec4(0.973f, 0.318f, 0.286f, 1.0f),  // 1 bright red
    ImVec4(0.341f, 0.894f, 0.400f, 1.0f),  // 2 bright green
    ImVec4(0.941f, 0.769f, 0.290f, 1.0f),  // 3 bright yellow
    ImVec4(0.475f, 0.753f, 1.000f, 1.0f),  // 4 bright blue
    ImVec4(0.847f, 0.694f, 1.000f, 1.0f),  // 5 bright magenta
    ImVec4(0.388f, 0.922f, 0.855f, 1.0f),  // 6 bright cyan
    ImVec4(1.000f, 1.000f, 1.000f, 1.0f),  // 7 bright white
};

// Render text with ANSI escape sequence support
// Handles: ESC[0m (reset), ESC[1m (bold), ESC[3m (italic/dim),
//          ESC[30-37m, ESC[90-97m (fg colors), ESC[38;5;Nm (256-color)
static void render_ansi_text(const char* text, ImVec4 default_color) {
    if (!text || !*text) return;

    ImVec4 current_color = default_color;
    bool bold = false;
    bool has_custom_color = false;
    const char* p = text;

    while (*p) {
        // Find next ESC or end
        const char* span_start = p;
        while (*p && !(*p == '\033' && *(p + 1) == '[')) {
            p++;
        }

        // Render text span if any
        if (p > span_start) {
            ImGui::PushStyleColor(ImGuiCol_Text, current_color);
            ImGui::TextUnformatted(span_start, p);
            ImGui::PopStyleColor();
            // SameLine for next span on same line (no newline between spans)
            if (*p) ImGui::SameLine(0, 0);
        }

        if (!*p) break;

        // Parse ESC[ ... m sequence
        p += 2;  // skip ESC[
        while (*p) {
            // Parse number
            int code = 0;
            bool has_num = false;
            while (*p >= '0' && *p <= '9') {
                code = code * 10 + (*p - '0');
                has_num = true;
                p++;
            }
            if (!has_num) code = 0;

            // Apply SGR code
            if (code == 0) {
                // Reset
                current_color = default_color;
                bold = false;
                has_custom_color = false;
            } else if (code == 1) {
                bold = true;
                // If we have a standard color, upgrade to bright
                if (!has_custom_color) {
                    current_color = default_color;
                }
            } else if (code == 2 || code == 3) {
                // Dim/italic — slightly reduce alpha
                current_color.w = 0.7f;
            } else if (code == 4 || code == 9) {
                // Underline/strikethrough — no ImGui support, ignore
            } else if (code >= 30 && code <= 37) {
                current_color = bold ? ansi_bright[code - 30] : ansi_colors[code - 30];
                has_custom_color = true;
            } else if (code == 39) {
                current_color = default_color;
                has_custom_color = false;
            } else if (code >= 90 && code <= 97) {
                current_color = ansi_bright[code - 90];
                has_custom_color = true;
            } else if (code == 38) {
                // Extended color: 38;5;N (256-color) or 38;2;R;G;B (truecolor)
                if (*p == ';') {
                    p++;
                    int mode = 0;
                    while (*p >= '0' && *p <= '9') { mode = mode * 10 + (*p - '0'); p++; }
                    if (mode == 5 && *p == ';') {
                        // 256-color
                        p++;
                        int idx = 0;
                        while (*p >= '0' && *p <= '9') { idx = idx * 10 + (*p - '0'); p++; }
                        if (idx < 8) {
                            current_color = ansi_colors[idx];
                        } else if (idx < 16) {
                            current_color = ansi_bright[idx - 8];
                        } else if (idx < 232) {
                            // 216-color cube: 16 + 36*r + 6*g + b
                            int v = idx - 16;
                            int r = v / 36; int g = (v % 36) / 6; int b = v % 6;
                            current_color = ImVec4(r / 5.0f, g / 5.0f, b / 5.0f, 1.0f);
                        } else {
                            // Grayscale: 232-255 -> 8..238
                            float gray = (float)(8 + (idx - 232) * 10) / 255.0f;
                            current_color = ImVec4(gray, gray, gray, 1.0f);
                        }
                        has_custom_color = true;
                    } else if (mode == 2 && *p == ';') {
                        // Truecolor: 38;2;R;G;B
                        int rgb[3] = {0, 0, 0};
                        for (int i = 0; i < 3 && *p == ';'; i++) {
                            p++;
                            while (*p >= '0' && *p <= '9') { rgb[i] = rgb[i] * 10 + (*p - '0'); p++; }
                        }
                        current_color = ImVec4(rgb[0] / 255.0f, rgb[1] / 255.0f, rgb[2] / 255.0f, 1.0f);
                        has_custom_color = true;
                    }
                }
            }

            if (*p == ';') {
                p++;  // more codes follow
            } else if (*p == 'm') {
                p++;  // end of sequence
                break;
            } else {
                // Malformed — skip to 'm' or end
                while (*p && *p != 'm') p++;
                if (*p == 'm') p++;
                break;
            }
        }
    }
}

// History navigation callback
static int input_callback(ImGuiInputTextCallbackData* data) {
    repl_state_t* state = (repl_state_t*)data->UserData;
    if (!state) return 0;

    if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory) {
        int history_size = (int)state->history.size();
        if (history_size == 0) return 0;

        if (state->history_pos == -1) {
            state->saved_input = std::string(data->Buf, data->BufTextLen);
        }

        if (data->EventKey == ImGuiKey_UpArrow) {
            if (state->history_pos == -1) {
                state->history_pos = history_size - 1;
            } else if (state->history_pos > 0) {
                state->history_pos--;
            }
        } else if (data->EventKey == ImGuiKey_DownArrow) {
            if (state->history_pos != -1) {
                state->history_pos++;
                if (state->history_pos >= history_size) {
                    state->history_pos = -1;
                }
            }
        }

        const char* new_text = (state->history_pos == -1)
            ? state->saved_input.c_str()
            : state->history[state->history_pos].c_str();

        data->DeleteChars(0, data->BufTextLen);
        data->InsertChars(0, new_text);
    }

    return 0;
}

extern "C" {

nil_t rfui_repl_init(nil_t) {
    if (g_repl) return;  // Already initialized

    g_repl = new repl_state_t();
    g_repl->input_buf[0] = '\0';
    g_repl->history_pos = -1;
    g_repl->scroll_to_bottom = true;
}

nil_t rfui_repl_render(nil_t) {
    if (!g_repl) return;

    repl_state_t* state = g_repl;

    // Colors (matched to theme palette)
    ImVec4 prompt_color(0.247f, 0.725f, 0.314f, 1.0f);  // #3FB950
    ImVec4 result_color(0.902f, 0.929f, 0.953f, 1.0f);  // #E6EDF3
    ImVec4 error_color(0.973f, 0.318f, 0.286f, 1.0f);   // #F85149

    // Single scrollable region for entire terminal
    ImGui::BeginChild("##terminal", ImVec2(0, 0), false,
                      ImGuiWindowFlags_HorizontalScrollbar);

    // Display all previous lines (with ANSI escape sequence support)
    for (const terminal_line_t& line : state->lines) {
        ImVec4 base_color;
        switch (line.type) {
            case LINE_INPUT: base_color = prompt_color; break;
            case LINE_ERROR: base_color = error_color;  break;
            default:         base_color = result_color;  break;
        }
        render_ansi_text(line.text.c_str(), base_color);
    }

    // Subtle separator between output and input
    if (!state->lines.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.188f, 0.212f, 0.239f, 1.0f));
        ImGui::Separator();
        ImGui::PopStyleColor();
    }

    // Current input line: prompt + input field on same line
    ImGui::PushStyleColor(ImGuiCol_Text, prompt_color);
    ImGui::TextUnformatted(ICON_PROMPT " ");
    ImGui::PopStyleColor();
    ImGui::SameLine(0, 0);

    // Make input field blend with terminal (no frame, no border, no highlight)
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_NavHighlight, ImVec4(0, 0, 0, 0));

    // Auto-focus input
    if (ImGui::IsWindowAppearing() || state->scroll_to_bottom) {
        ImGui::SetKeyboardFocusHere();
    }

    ImGui::SetNextItemWidth(-1);
    ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue |
                                 ImGuiInputTextFlags_CallbackHistory;

    bool enter_pressed = ImGui::InputText("##input", state->input_buf,
                                           sizeof(state->input_buf), flags,
                                           input_callback, state);

    ImGui::PopStyleColor(5);
    ImGui::PopStyleVar(2);

    // Handle Enter
    if (enter_pressed && state->input_buf[0] != '\0') {
        std::string input(state->input_buf);

        // Add to command history
        if (state->history.empty() || state->history.back() != input) {
            if ((int)state->history.size() >= MAX_HISTORY_SIZE) {
                state->history.erase(state->history.begin());
            }
            state->history.push_back(input);
        }

        // Add input line to terminal
        if ((int)state->lines.size() >= MAX_OUTPUT_LINES) {
            state->lines.erase(state->lines.begin());
        }
        state->lines.push_back({std::string(ICON_PROMPT " ") + input, LINE_INPUT});

        // Evaluate
        rfui_eval(state->input_buf);

        // Clear
        state->input_buf[0] = '\0';
        state->history_pos = -1;
        state->saved_input.clear();
        state->scroll_to_bottom = true;
    }

    // Auto-scroll
    if (state->scroll_to_bottom) {
        ImGui::SetScrollHereY(1.0f);
        state->scroll_to_bottom = false;
    }

    ImGui::EndChild();
}

nil_t rfui_repl_add_result_text(const char* text) {
    if (!g_repl || !text) return;

    // Determine if error (starts with "!" or "error")
    LineType type = LINE_RESULT;
    if (text[0] == '!' || strncmp(text, "error", 5) == 0) {
        type = LINE_ERROR;
    }

    if ((int)g_repl->lines.size() >= MAX_OUTPUT_LINES) {
        g_repl->lines.erase(g_repl->lines.begin());
    }
    g_repl->lines.push_back({std::string(text), type});
    g_repl->scroll_to_bottom = true;
}

nil_t rfui_repl_destroy(nil_t) {
    if (!g_repl) return;

    delete g_repl;
    g_repl = nullptr;
}

} // extern "C"
