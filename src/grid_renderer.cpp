// src/grid_renderer.cpp
// Grid widget renderer using ImGui with virtualization for Rayforce tables

#include <stdio.h>
#include <string.h>

#include "imgui.h"

// Make rayforce headers C++ compatible by redefining _Static_assert
#define _Static_assert static_assert

extern "C" {
#include "../include/raygui/grid_renderer.h"
#include "../include/raygui/widget.h"
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

extern "C" {

nil_t raygui_render_grid(raygui_widget_t* widget) {
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

    // Display table info
    ImGui::Text("Rows: %lld  Columns: %lld", (long long)nrows, (long long)ncols);
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
        ImGuiTableFlags_ScrollY;

    // Use available content region for table
    ImVec2 outer_size = ImVec2(0.0f, 0.0f);

    if (ImGui::BeginTable("##grid", (int)ncols, table_flags, outer_size)) {
        // Setup columns with headers
        for (i64_t col_idx = 0; col_idx < ncols; col_idx++) {
            i64_t sym_id = AS_SYMBOL(keys)[col_idx];
            const char* col_name = str_from_symbol(sym_id);

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

        while (clipper.Step()) {
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
                ImGui::TableNextRow();

                // Render each cell in the row
                for (i64_t col_idx = 0; col_idx < ncols; col_idx++) {
                    ImGui::TableSetColumnIndex((int)col_idx);

                    obj_p col = AS_LIST(vals)[col_idx];
                    if (col == nullptr) {
                        ImGui::TextDisabled("null");
                        continue;
                    }

                    // Verify row is within column bounds
                    if (row >= col->len) {
                        ImGui::TextDisabled("OOB");
                        continue;
                    }

                    // Render cell with zero-copy direct buffer access
                    render_cell(col, (i64_t)row);
                }
            }
        }

        clipper.End();
        ImGui::EndTable();
    }
}

} // extern "C"
