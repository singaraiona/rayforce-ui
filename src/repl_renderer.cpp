// src/repl_renderer.cpp
// Terminal-style REPL widget renderer

#include <stdio.h>
#include <string.h>
#include <vector>
#include <string>

#include "imgui.h"

// Scrollback limits
static const int MAX_HISTORY_SIZE = 1000;
static const int MAX_OUTPUT_LINES = 10000;

#define _Static_assert static_assert

extern "C" {
#include "../include/raygui/repl_renderer.h"
#include "../include/raygui/widget.h"
#include "../include/raygui/raygui.h"
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

nil_t raygui_render_repl(raygui_widget_t* widget) {
    if (widget == nullptr) return;

    // Initialize state
    if (widget->ui_state == nullptr) {
        repl_state_t* state = new repl_state_t();
        state->input_buf[0] = '\0';
        state->history_pos = -1;
        state->scroll_to_bottom = true;
        widget->ui_state = state;
    }

    repl_state_t* state = (repl_state_t*)widget->ui_state;

    // Colors
    ImVec4 prompt_color(0.3f, 0.8f, 0.3f, 1.0f);   // Green for prompt
    ImVec4 result_color(0.9f, 0.9f, 0.9f, 1.0f);   // White for results
    ImVec4 error_color(1.0f, 0.4f, 0.4f, 1.0f);    // Red for errors

    // Single scrollable region for entire terminal
    ImGui::BeginChild("##terminal", ImVec2(0, 0), false,
                      ImGuiWindowFlags_HorizontalScrollbar);

    // Display all previous lines
    for (const terminal_line_t& line : state->lines) {
        switch (line.type) {
            case LINE_INPUT:
                ImGui::PushStyleColor(ImGuiCol_Text, prompt_color);
                ImGui::TextUnformatted(line.text.c_str());
                ImGui::PopStyleColor();
                break;
            case LINE_RESULT:
                ImGui::PushStyleColor(ImGuiCol_Text, result_color);
                ImGui::TextUnformatted(line.text.c_str());
                ImGui::PopStyleColor();
                break;
            case LINE_ERROR:
                ImGui::PushStyleColor(ImGuiCol_Text, error_color);
                ImGui::TextUnformatted(line.text.c_str());
                ImGui::PopStyleColor();
                break;
        }
    }

    // Current input line: prompt + input field on same line
    ImGui::PushStyleColor(ImGuiCol_Text, prompt_color);
    ImGui::TextUnformatted("> ");
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
        state->lines.push_back({"> " + input, LINE_INPUT});

        // Evaluate
        raygui_eval(state->input_buf);

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

nil_t raygui_repl_add_result(raygui_widget_t* widget, const char* text) {
    if (widget == nullptr || text == nullptr) return;

    repl_state_t* state = (repl_state_t*)widget->ui_state;
    if (state == nullptr) return;

    // Determine if error (starts with "!" or "error")
    LineType type = LINE_RESULT;
    if (text[0] == '!' || strncmp(text, "error", 5) == 0) {
        type = LINE_ERROR;
    }

    if ((int)state->lines.size() >= MAX_OUTPUT_LINES) {
        state->lines.erase(state->lines.begin());
    }
    state->lines.push_back({std::string(text), type});
    state->scroll_to_bottom = true;
}

nil_t raygui_repl_free_state(raygui_widget_t* widget) {
    if (widget == nullptr || widget->ui_state == nullptr) return;

    repl_state_t* state = (repl_state_t*)widget->ui_state;
    delete state;
    widget->ui_state = nullptr;
}

} // extern "C"
