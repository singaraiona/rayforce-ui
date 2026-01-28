// src/chart_renderer.cpp
// Chart widget renderer using ImPlot for Rayforce tables

#include <stdio.h>
#include <string.h>

#include "imgui.h"
#include "implot.h"

// Make rayforce headers C++ compatible by redefining _Static_assert
#define _Static_assert static_assert

extern "C" {
#include "../include/raygui/chart_renderer.h"
#include "../include/raygui/widget.h"
}

// Helper function to check if a type is numeric and plottable
static bool is_numeric_type(i8_t type) {
    switch (type) {
        case TYPE_F64:
        case TYPE_I64:
        case TYPE_I32:
        case TYPE_I16:
        case TYPE_U8:
        case TYPE_B8:
            return true;
        default:
            return false;
    }
}

// Helper to plot a column as a line series with zero-copy where possible
static void plot_column_line(const char* name, obj_p col, i64_t nrows) {
    if (!col || nrows <= 0) return;

    switch (col->type) {
        case TYPE_F64:
            // Zero-copy: pass raw double pointer directly to ImPlot
            ImPlot::PlotLine(name, AS_F64(col), (int)nrows);
            break;

        case TYPE_I64: {
            // ImPlot has templated overloads for different types
            // For i64, we need to convert to double for plotting
            // Use PlotLineG with a getter callback for zero-copy
            struct GetterData {
                i64_t* data;
            };
            GetterData gd = { AS_I64(col) };
            ImPlot::PlotLineG(name,
                [](int idx, void* user_data) -> ImPlotPoint {
                    GetterData* d = (GetterData*)user_data;
                    return ImPlotPoint((double)idx, (double)d->data[idx]);
                },
                &gd, (int)nrows);
            break;
        }

        case TYPE_I32: {
            // Similar approach for i32
            struct GetterData {
                i32_t* data;
            };
            GetterData gd = { AS_I32(col) };
            ImPlot::PlotLineG(name,
                [](int idx, void* user_data) -> ImPlotPoint {
                    GetterData* d = (GetterData*)user_data;
                    return ImPlotPoint((double)idx, (double)d->data[idx]);
                },
                &gd, (int)nrows);
            break;
        }

        case TYPE_I16: {
            struct GetterData {
                i16_t* data;
            };
            GetterData gd = { AS_I16(col) };
            ImPlot::PlotLineG(name,
                [](int idx, void* user_data) -> ImPlotPoint {
                    GetterData* d = (GetterData*)user_data;
                    return ImPlotPoint((double)idx, (double)d->data[idx]);
                },
                &gd, (int)nrows);
            break;
        }

        case TYPE_U8: {
            struct GetterData {
                u8_t* data;
            };
            GetterData gd = { AS_U8(col) };
            ImPlot::PlotLineG(name,
                [](int idx, void* user_data) -> ImPlotPoint {
                    GetterData* d = (GetterData*)user_data;
                    return ImPlotPoint((double)idx, (double)d->data[idx]);
                },
                &gd, (int)nrows);
            break;
        }

        case TYPE_B8: {
            // Boolean as 0/1 values
            struct GetterData {
                b8_t* data;
            };
            GetterData gd = { AS_B8(col) };
            ImPlot::PlotLineG(name,
                [](int idx, void* user_data) -> ImPlotPoint {
                    GetterData* d = (GetterData*)user_data;
                    return ImPlotPoint((double)idx, d->data[idx] ? 1.0 : 0.0);
                },
                &gd, (int)nrows);
            break;
        }

        default:
            // Non-numeric types are skipped
            break;
    }
}

// Helper to plot a column as scatter points
static void plot_column_scatter(const char* name, obj_p col, i64_t nrows) {
    if (!col || nrows <= 0) return;

    switch (col->type) {
        case TYPE_F64:
            ImPlot::PlotScatter(name, AS_F64(col), (int)nrows);
            break;

        case TYPE_I64: {
            struct GetterData {
                i64_t* data;
            };
            GetterData gd = { AS_I64(col) };
            ImPlot::PlotScatterG(name,
                [](int idx, void* user_data) -> ImPlotPoint {
                    GetterData* d = (GetterData*)user_data;
                    return ImPlotPoint((double)idx, (double)d->data[idx]);
                },
                &gd, (int)nrows);
            break;
        }

        case TYPE_I32: {
            struct GetterData {
                i32_t* data;
            };
            GetterData gd = { AS_I32(col) };
            ImPlot::PlotScatterG(name,
                [](int idx, void* user_data) -> ImPlotPoint {
                    GetterData* d = (GetterData*)user_data;
                    return ImPlotPoint((double)idx, (double)d->data[idx]);
                },
                &gd, (int)nrows);
            break;
        }

        case TYPE_I16: {
            struct GetterData {
                i16_t* data;
            };
            GetterData gd = { AS_I16(col) };
            ImPlot::PlotScatterG(name,
                [](int idx, void* user_data) -> ImPlotPoint {
                    GetterData* d = (GetterData*)user_data;
                    return ImPlotPoint((double)idx, (double)d->data[idx]);
                },
                &gd, (int)nrows);
            break;
        }

        case TYPE_U8: {
            struct GetterData {
                u8_t* data;
            };
            GetterData gd = { AS_U8(col) };
            ImPlot::PlotScatterG(name,
                [](int idx, void* user_data) -> ImPlotPoint {
                    GetterData* d = (GetterData*)user_data;
                    return ImPlotPoint((double)idx, (double)d->data[idx]);
                },
                &gd, (int)nrows);
            break;
        }

        case TYPE_B8: {
            struct GetterData {
                b8_t* data;
            };
            GetterData gd = { AS_B8(col) };
            ImPlot::PlotScatterG(name,
                [](int idx, void* user_data) -> ImPlotPoint {
                    GetterData* d = (GetterData*)user_data;
                    return ImPlotPoint((double)idx, d->data[idx] ? 1.0 : 0.0);
                },
                &gd, (int)nrows);
            break;
        }

        default:
            break;
    }
}

// Helper to plot a column as bars
static void plot_column_bars(const char* name, obj_p col, i64_t nrows) {
    if (!col || nrows <= 0) return;

    switch (col->type) {
        case TYPE_F64:
            ImPlot::PlotBars(name, AS_F64(col), (int)nrows);
            break;

        case TYPE_I64: {
            struct GetterData {
                i64_t* data;
            };
            GetterData gd = { AS_I64(col) };
            ImPlot::PlotBarsG(name,
                [](int idx, void* user_data) -> ImPlotPoint {
                    GetterData* d = (GetterData*)user_data;
                    return ImPlotPoint((double)idx, (double)d->data[idx]);
                },
                &gd, (int)nrows, 0.67);
            break;
        }

        case TYPE_I32: {
            struct GetterData {
                i32_t* data;
            };
            GetterData gd = { AS_I32(col) };
            ImPlot::PlotBarsG(name,
                [](int idx, void* user_data) -> ImPlotPoint {
                    GetterData* d = (GetterData*)user_data;
                    return ImPlotPoint((double)idx, (double)d->data[idx]);
                },
                &gd, (int)nrows, 0.67);
            break;
        }

        case TYPE_I16: {
            struct GetterData {
                i16_t* data;
            };
            GetterData gd = { AS_I16(col) };
            ImPlot::PlotBarsG(name,
                [](int idx, void* user_data) -> ImPlotPoint {
                    GetterData* d = (GetterData*)user_data;
                    return ImPlotPoint((double)idx, (double)d->data[idx]);
                },
                &gd, (int)nrows, 0.67);
            break;
        }

        case TYPE_U8: {
            struct GetterData {
                u8_t* data;
            };
            GetterData gd = { AS_U8(col) };
            ImPlot::PlotBarsG(name,
                [](int idx, void* user_data) -> ImPlotPoint {
                    GetterData* d = (GetterData*)user_data;
                    return ImPlotPoint((double)idx, (double)d->data[idx]);
                },
                &gd, (int)nrows, 0.67);
            break;
        }

        case TYPE_B8: {
            struct GetterData {
                b8_t* data;
            };
            GetterData gd = { AS_B8(col) };
            ImPlot::PlotBarsG(name,
                [](int idx, void* user_data) -> ImPlotPoint {
                    GetterData* d = (GetterData*)user_data;
                    return ImPlotPoint((double)idx, d->data[idx] ? 1.0 : 0.0);
                },
                &gd, (int)nrows, 0.67);
            break;
        }

        default:
            break;
    }
}

// Helper to extract a double from a numeric column at a given index
static double get_numeric_value(obj_p col, i64_t idx) {
    switch (col->type) {
        case TYPE_F64: return AS_F64(col)[idx];
        case TYPE_I64: return (double)AS_I64(col)[idx];
        case TYPE_I32: return (double)AS_I32(col)[idx];
        case TYPE_I16: return (double)AS_I16(col)[idx];
        case TYPE_U8:  return (double)AS_U8(col)[idx];
        case TYPE_B8:  return AS_B8(col)[idx] ? 1.0 : 0.0;
        default:       return 0.0;
    }
}

// Find a column index by name. Returns -1 if not found.
static i64_t find_column(i64_t* sym_ids, i64_t ncols, const char* name) {
    for (i64_t i = 0; i < ncols; i++) {
        const char* col_name = str_from_symbol(sym_ids[i]);
        if (col_name && strcmp(col_name, name) == 0) return i;
    }
    return -1;
}

// Plot candlestick chart. Expects open/close/low/high columns.
static void plot_candlestick(obj_p* cols, i64_t* sym_ids, i64_t ncols, i64_t nrows) {
    i64_t oi = find_column(sym_ids, ncols, "open");
    i64_t ci = find_column(sym_ids, ncols, "close");
    i64_t li = find_column(sym_ids, ncols, "low");
    i64_t hi = find_column(sym_ids, ncols, "high");

    if (oi < 0 || ci < 0 || li < 0 || hi < 0) {
        ImGui::TextDisabled("Candlestick requires open/close/low/high columns");
        return;
    }

    obj_p open_col  = cols[oi];
    obj_p close_col = cols[ci];
    obj_p low_col   = cols[li];
    obj_p high_col  = cols[hi];

    if (!open_col || !close_col || !low_col || !high_col) return;
    if (!is_numeric_type(open_col->type) || !is_numeric_type(close_col->type) ||
        !is_numeric_type(low_col->type)  || !is_numeric_type(high_col->type)) return;

    ImU32 bull_col = IM_COL32(63, 185, 80, 255);   // #3FB950
    ImU32 bear_col = IM_COL32(248, 81, 73, 255);    // #F85149

    float half_width = 0.35f;
    ImDrawList* draw_list = ImPlot::GetPlotDrawList();

    for (i64_t i = 0; i < nrows; i++) {
        double o = get_numeric_value(open_col, i);
        double c = get_numeric_value(close_col, i);
        double l = get_numeric_value(low_col, i);
        double h = get_numeric_value(high_col, i);
        double x = (double)i;

        bool bull = c >= o;
        ImU32 color = bull ? bull_col : bear_col;

        // Wick (high-low line)
        ImVec2 wick_lo = ImPlot::PlotToPixels(ImPlotPoint(x, l));
        ImVec2 wick_hi = ImPlot::PlotToPixels(ImPlotPoint(x, h));
        draw_list->AddLine(wick_lo, wick_hi, color, 1.0f);

        // Body (open-close rect)
        ImVec2 body_lo = ImPlot::PlotToPixels(ImPlotPoint(x - half_width, bull ? o : c));
        ImVec2 body_hi = ImPlot::PlotToPixels(ImPlotPoint(x + half_width, bull ? c : o));
        draw_list->AddRectFilled(body_lo, body_hi, color);
    }
}

extern "C" {

nil_t raygui_render_chart(raygui_widget_t* widget) {
    if (widget == nullptr) {
        return;
    }

    obj_p table = widget->render_data;

    // Check if we have valid table data
    if (table == nullptr) {
        ImGui::TextDisabled("No chart data");
        return;
    }

    if (table->type != TYPE_TABLE) {
        ImGui::TextDisabled("Chart requires table data (got %s)", type_name(table->type));
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

    // Count numeric columns for display info
    i64_t numeric_cols = 0;
    for (i64_t i = 0; i < ncols; i++) {
        obj_p col = AS_LIST(vals)[i];
        if (col && is_numeric_type(col->type)) {
            numeric_cols++;
        }
    }

    // Display chart info with secondary text color
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.545f, 0.580f, 0.620f, 1.0f));
    ImGui::Text("Points: %lld  Series: %lld", (long long)nrows, (long long)numeric_cols);
    ImGui::PopStyleColor();

    // Chart type selector (stored in widget ui_state if needed for persistence)
    // For now, default to line chart
    static int chart_type = 0; // 0=Line, 1=Scatter, 2=Bar
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    ImGui::Combo("##charttype", &chart_type, "Line\0Scatter\0Bar\0Candlestick\0");

    ImGui::Separator();

    // Create ImPlot chart
    // Use ImPlotFlags_None for default interactive plot
    if (ImPlot::BeginPlot(widget->name, ImVec2(-1, -1), ImPlotFlags_None)) {
        // Setup axes with auto-fit on first frame
        ImPlot::SetupAxes("X", "Y", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);

        // Cache column pointers for performance
        obj_p* cols = AS_LIST(vals);
        i64_t* sym_ids = AS_SYMBOL(keys);

        if (chart_type == 3) {
            // Candlestick uses multiple columns together
            plot_candlestick(cols, sym_ids, ncols, nrows);
        } else {
            // Plot each numeric column
            for (i64_t col_idx = 0; col_idx < ncols; col_idx++) {
                obj_p col = cols[col_idx];
                if (col == nullptr) continue;
                if (!is_numeric_type(col->type)) continue;
                if (col->len != nrows) continue;

                const char* col_name = str_from_symbol(sym_ids[col_idx]);
                if (!col_name) col_name = "<unknown>";

                switch (chart_type) {
                    case 0: plot_column_line(col_name, col, nrows); break;
                    case 1: plot_column_scatter(col_name, col, nrows); break;
                    case 2: plot_column_bars(col_name, col, nrows); break;
                }
            }
        }

        ImPlot::EndPlot();
    }
}

} // extern "C"
