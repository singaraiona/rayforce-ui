// src/repl_renderer.cpp
// REPL widget renderer with input history and output display

#include <stdio.h>
#include <string.h>
#include <vector>
#include <string>

#include "imgui.h"

// Scrollback limits to prevent unbounded memory growth
static const int MAX_HISTORY_SIZE = 1000;
static const int MAX_OUTPUT_LINES = 10000;

// Make rayforce headers C++ compatible by redefining _Static_assert
#define _Static_assert static_assert

extern "C" {
#include "../include/raygui/repl_renderer.h"
#include "../include/raygui/widget.h"
#include "../include/raygui/raygui.h"
}

// REPL UI state structure (stored in widget->ui_state)
struct repl_state_t {
    char input_buf[4096];              // Current input buffer
    std::vector<std::string> history;  // Command history
    std::vector<std::string> output;   // Output lines
    int history_pos;                   // Current position in history (-1 = new input)
    bool scroll_to_bottom;             // Flag to auto-scroll
    bool refocus_input;                // Flag to refocus input field next frame
    std::string saved_input;           // Saved input when browsing history
};

// Input callback for handling history navigation
static int input_callback(ImGuiInputTextCallbackData* data) {
    repl_state_t* state = (repl_state_t*)data->UserData;
    if (!state) return 0;

    if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory) {
        // Get current history size
        int history_size = (int)state->history.size();
        if (history_size == 0) return 0;

        // Save current input when starting to browse history
        if (state->history_pos == -1) {
            state->saved_input = std::string(data->Buf, data->BufTextLen);
        }

        if (data->EventKey == ImGuiKey_UpArrow) {
            // Move back in history
            if (state->history_pos == -1) {
                // Start browsing from most recent
                state->history_pos = history_size - 1;
            } else if (state->history_pos > 0) {
                state->history_pos--;
            }
        } else if (data->EventKey == ImGuiKey_DownArrow) {
            // Move forward in history
            if (state->history_pos != -1) {
                state->history_pos++;
                if (state->history_pos >= history_size) {
                    // Restore saved input
                    state->history_pos = -1;
                }
            }
        }

        // Update input buffer with history entry or saved input
        const char* new_text;
        if (state->history_pos == -1) {
            new_text = state->saved_input.c_str();
        } else {
            new_text = state->history[state->history_pos].c_str();
        }

        data->DeleteChars(0, data->BufTextLen);
        data->InsertChars(0, new_text);
    }

    return 0;
}

extern "C" {

nil_t raygui_render_repl(raygui_widget_t* widget) {
    if (widget == nullptr) {
        return;
    }

    // Check/create ui_state if null
    if (widget->ui_state == nullptr) {
        repl_state_t* state = new repl_state_t();
        state->input_buf[0] = '\0';
        state->history_pos = -1;
        state->scroll_to_bottom = false;
        state->refocus_input = false;
        widget->ui_state = state;
    }

    repl_state_t* state = (repl_state_t*)widget->ui_state;

    // Get available content region
    ImVec2 content_size = ImGui::GetContentRegionAvail();

    // Reserve space for input area at bottom (input field + some padding)
    float input_height = ImGui::GetFrameHeightWithSpacing() + 4.0f;
    float output_height = content_size.y - input_height;

    // Output area (scrollable)
    if (output_height > 0) {
        ImGui::BeginChild("##repl_output", ImVec2(0, output_height), true,
                          ImGuiWindowFlags_HorizontalScrollbar);

        // Display each output line
        for (const std::string& line : state->output) {
            // Color prompt lines differently
            if (line.length() > 0 && line[0] == '>') {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
                ImGui::TextWrapped("%s", line.c_str());
                ImGui::PopStyleColor();
            } else if (line.length() > 0 && line[0] == '!') {
                // Error lines
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
                ImGui::TextWrapped("%s", line.c_str());
                ImGui::PopStyleColor();
            } else {
                ImGui::TextWrapped("%s", line.c_str());
            }
        }

        // Auto-scroll to bottom when new output added
        if (state->scroll_to_bottom) {
            ImGui::SetScrollHereY(1.0f);
            state->scroll_to_bottom = false;
        }

        ImGui::EndChild();
    }

    // Input area
    ImGui::Separator();

    // Set input field width to fill available space minus a small margin
    ImGui::SetNextItemWidth(-1);

    // Set focus to input field on first frame or after Enter was pressed
    if (ImGui::IsWindowAppearing() || state->refocus_input) {
        ImGui::SetKeyboardFocusHere(0);  // Focus the next widget (InputText)
        state->refocus_input = false;
    }

    // Input field with history callback
    ImGuiInputTextFlags input_flags = ImGuiInputTextFlags_EnterReturnsTrue |
                                       ImGuiInputTextFlags_CallbackHistory;

    bool enter_pressed = ImGui::InputText("##repl_input", state->input_buf,
                                           sizeof(state->input_buf), input_flags,
                                           input_callback, state);

    // Handle Enter key
    if (enter_pressed && state->input_buf[0] != '\0') {
        std::string input(state->input_buf);

        // Add to history (avoid duplicates of last entry)
        if (state->history.empty() || state->history.back() != input) {
            // Enforce history size limit
            if ((int)state->history.size() >= MAX_HISTORY_SIZE) {
                state->history.erase(state->history.begin());
            }
            state->history.push_back(input);
        }

        // Enforce output size limit before adding new line
        if ((int)state->output.size() >= MAX_OUTPUT_LINES) {
            state->output.erase(state->output.begin());
        }

        // Add "> input" to output
        state->output.push_back("> " + input);

        // Send to Rayforce for evaluation
        raygui_eval(state->input_buf);

        // Clear input buffer
        state->input_buf[0] = '\0';

        // Reset history position
        state->history_pos = -1;
        state->saved_input.clear();

        // Scroll to bottom
        state->scroll_to_bottom = true;

        // Request focus for next frame (SetKeyboardFocusHere must be called before InputText)
        state->refocus_input = true;
    }

    return;
}

nil_t raygui_repl_add_result(raygui_widget_t* widget, const char* text) {
    if (widget == nullptr || text == nullptr) {
        return;
    }

    // Get repl_state from widget->ui_state
    repl_state_t* state = (repl_state_t*)widget->ui_state;
    if (state == nullptr) {
        return;
    }

    // Enforce output size limit before adding new line
    if ((int)state->output.size() >= MAX_OUTPUT_LINES) {
        state->output.erase(state->output.begin());
    }

    // Append text to output vector
    state->output.push_back(std::string(text));

    // Set scroll_to_bottom flag
    state->scroll_to_bottom = true;
}

nil_t raygui_repl_free_state(raygui_widget_t* widget) {
    if (widget == nullptr || widget->ui_state == nullptr) {
        return;
    }

    // Delete C++ state object (invokes destructors for vectors/strings)
    repl_state_t* state = (repl_state_t*)widget->ui_state;
    delete state;

    // Set to null so widget_destroy doesn't try to free() it
    widget->ui_state = nullptr;
}

} // extern "C"
