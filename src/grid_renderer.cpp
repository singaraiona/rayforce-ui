// src/grid_renderer.cpp
// Grid widget renderer using ImGui with virtualization for Rayforce tables

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "imgui.h"
#include "../include/rfui/icons.h"

// Make rayforce headers C++ compatible by redefining _Static_assert
#define _Static_assert static_assert

extern "C" {
#include "../include/rfui/grid_renderer.h"
#include "../include/rfui/widget.h"
#include "../include/rfui/context.h"
#include "../include/rfui/message.h"
#include "../include/rfui/queue.h"
#include "../deps/rayforce/core/poll.h"

// Global context declared in main.c
extern rfui_ctx_t* g_ctx;
}

#define MAX_COLOR_RULES 8

typedef struct color_rule_t {
    char column[64];     // Column name to match
    char value[64];      // Value to match (string comparison)
    ImVec4 color;        // Text color when matched
    bool enabled;
} color_rule_t;

// UI state for grid selection (stored in widget->ui_state)
typedef struct grid_ui_state_t {
    int selected_row;  // -1 = no selection
    color_rule_t color_rules[MAX_COLOR_RULES];
    int num_rules;
    bool settings_open;
} grid_ui_state_t;

// Helper to send MSG_SET_POST_QUERY to Rayforce thread
static void send_post_query(rfui_widget_t* widget, const char* expr) {
    if (!g_ctx || !widget) return;

    // Allocate message
    rfui_ui_msg_t* msg = (rfui_ui_msg_t*)malloc(sizeof(rfui_ui_msg_t));
    if (!msg) return;

    // Duplicate expression string
    char* expr_copy = nullptr;
    if (expr) {
        size_t len = strlen(expr);
        expr_copy = (char*)malloc(len + 1);
        if (!expr_copy) {
            free(msg);
            return;
        }
        memcpy(expr_copy, expr, len + 1);
    }

    msg->type = RFUI_MSG_SET_POST_QUERY;
    msg->expr = expr_copy;
    msg->obj = nullptr;
    msg->widget = widget;

    // Push to queue
    if (!rfui_queue_push(g_ctx->ui_to_ray, msg)) {
        if (expr_copy) free(expr_copy);
        free(msg);
        return;
    }

    // Wake Rayforce thread
    poll_waker_p waker = rfui_ctx_get_waker(g_ctx);
    if (waker) {
        poll_waker_wake(waker);
    }
}

// Build a filter expression for the selected row
// Expression: {[x] (take 1 (drop ROW_INDEX x))}
// This creates a lambda that takes data and returns just the selected row
static char* build_row_filter_expr(int row_index) {
    // Max length: "{[x] (take 1 (drop 999999999 x))}" = ~40 chars
    char* buf = (char*)malloc(64);
    if (!buf) return nullptr;
    snprintf(buf, 64, "{[x] (take 1 (drop %d x))}", row_index);
    return buf;
}

// Helper function to render a single cell based on column type
static void render_cell(obj_p col, i64_t row) {
    if (col == nullptr || row < 0 || row >= col->len) {
        ImGui::Text("?");
        return;
    }

    switch (col->type) {
        case TYPE_I64:
            ImGui::Text("%lld", (long long)AS_I64(col)[row]);
            break;
        case TYPE_I32:
            ImGui::Text("%d", AS_I32(col)[row]);
            break;
        case TYPE_I16:
            ImGui::Text("%d", (int)AS_I16(col)[row]);
            break;
        case TYPE_F64: {
            f64_t val = AS_F64(col)[row];
            // Check for NaN (null value in Rayforce)
            if (val != val) {
                ImGui::TextDisabled("null");
            } else {
                ImGui::Text("%.6g", val);
            }
            break;
        }
        case TYPE_SYMBOL: {
            i64_t sid = AS_SYMBOL(col)[row];
            const char* str = str_from_symbol(sid);
            if (str) {
                ImGui::Text("%s", str);
            } else {
                ImGui::TextDisabled("null");
            }
            break;
        }
        case TYPE_B8:
            ImGui::Text("%s", AS_B8(col)[row] ? "true" : "false");
            break;
        case TYPE_U8:
            ImGui::Text("%u", (unsigned)AS_U8(col)[row]);
            break;
        case TYPE_C8: {
            // Single character or string
            char c = AS_C8(col)[row];
            if (c >= 32 && c < 127) {
                ImGui::Text("%c", c);
            } else {
                ImGui::Text("0x%02x", (unsigned char)c);
            }
            break;
        }
        case TYPE_DATE: {
            // Date stored as i32 (days since epoch)
            i32_t d = AS_DATE(col)[row];
            if (d == NULL_I32) {
                ImGui::TextDisabled("null");
            } else {
                // Simple date format: YYYY.MM.DD
                // Days since 2000-01-01
                ImGui::Text("%d", d);
            }
            break;
        }
        case TYPE_TIME: {
            // Time stored as i32 (milliseconds since midnight)
            i32_t t = AS_TIME(col)[row];
            if (t == NULL_I32) {
                ImGui::TextDisabled("null");
            } else {
                int ms = t % 1000;
                int sec = (t / 1000) % 60;
                int min = (t / 60000) % 60;
                int hr = t / 3600000;
                ImGui::Text("%02d:%02d:%02d.%03d", hr, min, sec, ms);
            }
            break;
        }
        case TYPE_TIMESTAMP: {
            // Timestamp stored as i64 (nanoseconds since epoch)
            i64_t ts = AS_TIMESTAMP(col)[row];
            if (ts == NULL_I64) {
                ImGui::TextDisabled("null");
            } else {
                // Display as raw value for now
                ImGui::Text("%lld", (long long)ts);
            }
            break;
        }
        case TYPE_GUID: {
            // GUID is 16 bytes
            guid_t* g = &AS_GUID(col)[row];
            ImGui::Text("%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                (*g)[0], (*g)[1], (*g)[2], (*g)[3],
                (*g)[4], (*g)[5], (*g)[6], (*g)[7],
                (*g)[8], (*g)[9], (*g)[10], (*g)[11],
                (*g)[12], (*g)[13], (*g)[14], (*g)[15]);
            break;
        }
        case TYPE_LIST: {
            // Nested list - show type indicator
            obj_p item = AS_LIST(col)[row];
            if (item) {
                ImGui::TextDisabled("[%s:%lld]", type_name(item->type), (long long)item->len);
            } else {
                ImGui::TextDisabled("null");
            }
            break;
        }
        default:
            ImGui::TextDisabled("<%s>", type_name(col->type));
            break;
    }
}

// Get cell value as string for color rule matching
static void cell_to_string(obj_p col, i64_t row, char* buf, size_t buf_sz) {
    buf[0] = '\0';
    if (!col || row < 0 || row >= col->len) return;

    switch (col->type) {
        case TYPE_I64:     snprintf(buf, buf_sz, "%lld", (long long)AS_I64(col)[row]); break;
        case TYPE_I32:     snprintf(buf, buf_sz, "%d", AS_I32(col)[row]); break;
        case TYPE_I16:     snprintf(buf, buf_sz, "%d", (int)AS_I16(col)[row]); break;
        case TYPE_F64:     snprintf(buf, buf_sz, "%.6g", AS_F64(col)[row]); break;
        case TYPE_SYMBOL: {
            i64_t sid = AS_SYMBOL(col)[row];
            const char* s = str_from_symbol(sid);
            if (s) snprintf(buf, buf_sz, "%s", s);
            break;
        }
        case TYPE_B8:      snprintf(buf, buf_sz, "%s", AS_B8(col)[row] ? "true" : "false"); break;
        default: break;
    }
}

extern "C" {

// Note: render_data lifetime is managed by the widget registry and must remain
// valid during render. The caller is responsible for ensuring render_data
// points to valid memory throughout the widget's lifecycle.
nil_t rfui_render_grid(rfui_widget_t* widget) {
    if (widget == nullptr) {
        return;
    }
    obj_p table = widget->render_data;

    // Check if we have valid table data
    if (table == nullptr || table->type != TYPE_TABLE) {
        ImGui::TextDisabled("No table data");
        return;
    }

    // Table structure: list of [keys, vals]
    // keys: symbol vector of column names
    // vals: list of column vectors
    if (table->len < 2) {
        ImGui::TextDisabled("Invalid table structure");
        return;
    }

    obj_p keys = AS_LIST(table)[0];  // Symbol vector of column names
    obj_p vals = AS_LIST(table)[1];  // List of column vectors

    if (keys == nullptr || vals == nullptr) {
        ImGui::TextDisabled("Table has null keys or values");
        return;
    }

    // Validate types
    if (keys->type != TYPE_SYMBOL) {
        ImGui::TextDisabled("Table keys must be symbols (got %s)", type_name(keys->type));
        return;
    }

    if (vals->type != TYPE_LIST) {
        ImGui::TextDisabled("Table values must be a list (got %s)", type_name(vals->type));
        return;
    }

    i64_t ncols = keys->len;
    if (ncols == 0) {
        ImGui::TextDisabled("Table has no columns");
        return;
    }

    // Check column count matches
    if (vals->len != ncols) {
        ImGui::TextDisabled("Column count mismatch: %lld keys vs %lld values",
            (long long)ncols, (long long)vals->len);
        return;
    }

    // Get row count from first column
    obj_p first_col = AS_LIST(vals)[0];
    if (first_col == nullptr) {
        ImGui::TextDisabled("First column is null");
        return;
    }
    i64_t nrows = first_col->len;
    if (nrows == 0) {
        ImGui::TextDisabled("Empty table (0 rows)");
        return;
    }

    // Validate all columns have the same length
    for (i64_t col_idx = 1; col_idx < ncols; col_idx++) {
        obj_p col = AS_LIST(vals)[col_idx];
        if (col == nullptr) {
            ImGui::TextDisabled("Column %lld is null", (long long)col_idx);
            return;
        }
        if (col->len != nrows) {
            ImGui::TextDisabled("Column %lld length mismatch: %lld vs %lld",
                (long long)col_idx, (long long)col->len, (long long)nrows);
            return;
        }
    }

    // Initialize UI state if needed
    grid_ui_state_t* ui_state = (grid_ui_state_t*)widget->ui_state;
    if (!ui_state) {
        ui_state = (grid_ui_state_t*)malloc(sizeof(grid_ui_state_t));
        if (ui_state) {
            ui_state->selected_row = -1;
            ui_state->num_rules = 0;
            ui_state->settings_open = false;
            widget->ui_state = ui_state;
        }
    }

    // Display table info with secondary text color
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.545f, 0.580f, 0.620f, 1.0f));
    if (ui_state && ui_state->selected_row >= 0) {
        ImGui::Text("Rows: %lld  Columns: %lld  Selected: %d",
                    (long long)nrows, (long long)ncols, ui_state->selected_row);
        ImGui::SameLine();
        ImGui::PopStyleColor();
        if (ImGui::SmallButton(ICON_ERASER " Clear")) {
            ui_state->selected_row = -1;
            send_post_query(widget, nullptr);
        }
    } else {
        ImGui::Text("Rows: %lld  Columns: %lld", (long long)nrows, (long long)ncols);
        ImGui::PopStyleColor();
    }

    // Settings button
    if (ui_state) {
        ImGui::SameLine();
        if (ImGui::SmallButton(ICON_GEAR " Settings")) {
            ImGui::OpenPopup("GridSettings");
        }

        if (ImGui::BeginPopup("GridSettings")) {
            ImGui::Text(ICON_PALETTE " Color Rules");
            ImGui::Separator();

            for (int i = 0; i < ui_state->num_rules; i++) {
                color_rule_t* r = &ui_state->color_rules[i];
                ImGui::PushID(i);

                // Column combo
                ImGui::SetNextItemWidth(100);
                if (ImGui::BeginCombo("##col", r->column[0] ? r->column : "<column>")) {
                    for (i64_t c = 0; c < ncols; c++) {
                        const char* name = str_from_symbol(AS_SYMBOL(keys)[c]);
                        if (name && ImGui::Selectable(name, strcmp(r->column, name) == 0)) {
                            snprintf(r->column, sizeof(r->column), "%s", name);
                        }
                    }
                    ImGui::EndCombo();
                }

                ImGui::SameLine();
                ImGui::SetNextItemWidth(80);
                ImGui::InputText("##val", r->value, sizeof(r->value));

                ImGui::SameLine();
                ImGui::ColorEdit3("##clr", (float*)&r->color,
                    ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);

                ImGui::SameLine();
                ImGui::Checkbox("##en", &r->enabled);

                ImGui::SameLine();
                if (ImGui::SmallButton(ICON_XMARK)) {
                    // Remove rule by shifting
                    for (int j = i; j < ui_state->num_rules - 1; j++)
                        ui_state->color_rules[j] = ui_state->color_rules[j + 1];
                    ui_state->num_rules--;
                    i--;
                }

                ImGui::PopID();
            }

            if (ui_state->num_rules < MAX_COLOR_RULES) {
                if (ImGui::Button(ICON_PLUS " Add Rule")) {
                    color_rule_t* r = &ui_state->color_rules[ui_state->num_rules++];
                    r->column[0] = '\0';
                    r->value[0] = '\0';
                    r->color = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
                    r->enabled = true;
                }
            }

            ImGui::EndPopup();
        }
    }

    ImGui::Separator();

    // Create ImGui table with virtualization
    ImGuiTableFlags table_flags =
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_Reorderable |
        ImGuiTableFlags_Hideable |
        ImGuiTableFlags_Sortable |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_Borders |
        ImGuiTableFlags_ScrollX |
        ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_SizingFixedFit;  // Required for specifying column widths

    // Use available content region for table
    ImVec2 outer_size = ImVec2(0.0f, 0.0f);

    if (ImGui::BeginTable("##grid", (int)ncols, table_flags, outer_size)) {
        // Setup columns with headers
        for (i64_t col_idx = 0; col_idx < ncols; col_idx++) {
            i64_t sym_id = AS_SYMBOL(keys)[col_idx];
            const char* col_name = str_from_symbol(sym_id);
            if (!col_name) col_name = "<invalid>";

            // Get column type for sizing hints
            obj_p col = AS_LIST(vals)[col_idx];
            ImGuiTableColumnFlags col_flags = ImGuiTableColumnFlags_None;

            // Set initial column width based on type
            float init_width = 0.0f;  // 0 = auto
            if (col) {
                switch (col->type) {
                    case TYPE_B8:
                        init_width = 50.0f;
                        break;
                    case TYPE_I16:
                    case TYPE_I32:
                        init_width = 80.0f;
                        break;
                    case TYPE_I64:
                    case TYPE_TIMESTAMP:
                        init_width = 120.0f;
                        break;
                    case TYPE_F64:
                        init_width = 100.0f;
                        break;
                    case TYPE_DATE:
                        init_width = 90.0f;
                        break;
                    case TYPE_TIME:
                        init_width = 100.0f;
                        break;
                    case TYPE_SYMBOL:
                    case TYPE_C8:
                        init_width = 120.0f;
                        break;
                    case TYPE_GUID:
                        init_width = 280.0f;
                        break;
                    default:
                        init_width = 100.0f;
                        break;
                }
            }

            ImGui::TableSetupColumn(col_name ? col_name : "?", col_flags, init_width);
        }

        // Freeze header row
        ImGui::TableSetupScrollFreeze(0, 1);

        // Display headers
        ImGui::TableHeadersRow();

        // Use ListClipper for virtualized row rendering
        ImGuiListClipper clipper;
        clipper.Begin((int)nrows);

        // Cache column pointers for performance (avoid repeated AS_LIST dereference)
        obj_p* cols = AS_LIST(vals);

        // Pre-resolve color rule column indices
        int rule_col_idx[MAX_COLOR_RULES];
        if (ui_state) {
            for (int ri = 0; ri < ui_state->num_rules; ri++) {
                rule_col_idx[ri] = -1;
                if (!ui_state->color_rules[ri].enabled || !ui_state->color_rules[ri].column[0])
                    continue;
                for (i64_t c = 0; c < ncols; c++) {
                    const char* name = str_from_symbol(AS_SYMBOL(keys)[c]);
                    if (name && strcmp(name, ui_state->color_rules[ri].column) == 0) {
                        rule_col_idx[ri] = (int)c;
                        break;
                    }
                }
            }
        }

        while (clipper.Step()) {
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
                ImGui::TableNextRow();

                // Check if this row is selected
                bool is_selected = (ui_state && ui_state->selected_row == row);

                // Render each cell in the row
                for (i64_t col_idx = 0; col_idx < ncols; col_idx++) {
                    ImGui::TableSetColumnIndex((int)col_idx);

                    obj_p col = cols[col_idx];
                    if (col == nullptr) {
                        ImGui::TextDisabled("null");
                        continue;
                    }

                    // Verify row is within column bounds
                    if (row >= col->len) {
                        ImGui::TextDisabled("OOB");
                        continue;
                    }

                    // For the first column, add a selectable that spans all columns
                    if (col_idx == 0) {
                        // Create unique ID for this row's selectable
                        char selectable_id[32];
                        snprintf(selectable_id, sizeof(selectable_id), "##row%d", row);

                        // Selectable spans all columns and is drawn behind cell content
                        if (ImGui::Selectable(selectable_id, is_selected,
                                              ImGuiSelectableFlags_SpanAllColumns |
                                              ImGuiSelectableFlags_AllowOverlap)) {
                            // Row was clicked
                            if (ui_state) {
                                if (ui_state->selected_row == row) {
                                    // Clicking same row deselects
                                    ui_state->selected_row = -1;
                                    send_post_query(widget, nullptr);
                                } else {
                                    // Select new row
                                    ui_state->selected_row = row;
                                    char* expr = build_row_filter_expr(row);
                                    if (expr) {
                                        send_post_query(widget, expr);
                                        free(expr);
                                    }
                                }
                            }
                        }
                        ImGui::SameLine();
                    }

                    // Check color rules for this cell
                    bool cell_colored = false;
                    if (ui_state) {
                        for (int ri = 0; ri < ui_state->num_rules; ri++) {
                            color_rule_t* r = &ui_state->color_rules[ri];
                            if (!r->enabled || rule_col_idx[ri] != (int)col_idx) continue;
                            char cell_buf[128];
                            cell_to_string(col, (i64_t)row, cell_buf, sizeof(cell_buf));
                            if (strcmp(cell_buf, r->value) == 0) {
                                ImGui::PushStyleColor(ImGuiCol_Text, r->color);
                                cell_colored = true;
                                break;
                            }
                        }
                    }

                    // Render cell with zero-copy direct buffer access
                    render_cell(col, (i64_t)row);

                    if (cell_colored)
                        ImGui::PopStyleColor();
                }
            }
        }

        clipper.End();
        ImGui::EndTable();
    }
}

} // extern "C"
