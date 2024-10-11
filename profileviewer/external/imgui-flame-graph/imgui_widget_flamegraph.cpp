// The MIT License(MIT)
//
// Copyright(c) 2019 Sandy Carter
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <imgui_widget_flamegraph.h>

#include "imgui.h"
#include "imgui_internal.h"

static float s_max_timeline_value;


bool BeginTimeline(const char* str_id, float max_value) {
    s_max_timeline_value = max_value;
    return ImGui::BeginChild(str_id);
}


static const float TIMELINE_RADIUS = 6;


bool TimelineEvent(const char* str_id, float* values) {
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    const ImU32 inactive_color = ImGui::ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_Button]);
    const ImU32 active_color = ImGui::ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_ButtonHovered]);
    const ImU32 line_color = ImGui::ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_SeparatorActive]);
    bool changed = false;
    ImVec2 cursor_pos = win->DC.CursorPos;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    
    for (int i = 0; i < 2; ++i) {
        ImVec2 pos = cursor_pos;
        pos.x += (win->Size.x - 2*style.WindowPadding.x) * values[i] / s_max_timeline_value + TIMELINE_RADIUS;
        pos.y += TIMELINE_RADIUS;

        ImGui::SetCursorScreenPos(pos - ImVec2(TIMELINE_RADIUS, TIMELINE_RADIUS));
        ImGui::PushID(i);
        ImGui::InvisibleButton(str_id, ImVec2(2 * TIMELINE_RADIUS, 2 * TIMELINE_RADIUS));
        if (ImGui::IsItemActive() || ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%lld", (long long)values[i]);
            ImVec2 a(pos.x, ImGui::GetWindowContentRegionMin().y + win->Pos.y);
            ImVec2 b(pos.x, ImGui::GetWindowContentRegionMax().y + win->Pos.y);
            win->DrawList->AddLine(a, b, line_color);
        }
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
            values[i] += ImGui::GetIO().MouseDelta.x / win->Size.x * s_max_timeline_value;
            changed = true;
        }
        ImGui::PopID();
        win->DrawList->AddCircleFilled(
            pos, TIMELINE_RADIUS, ImGui::IsItemActive() || ImGui::IsItemHovered() ? active_color : inactive_color);
    }
    
    ImVec2 start = cursor_pos;
    start.x += (win->Size.x - 2*style.WindowPadding.x) * values[0] / s_max_timeline_value + 2 * TIMELINE_RADIUS;
    start.y += TIMELINE_RADIUS * 0.5f;
    ImVec2 end = start + ImVec2((win->Size.x - 2*style.WindowPadding.x) * (values[1] - values[0]) / s_max_timeline_value - 2 * TIMELINE_RADIUS,
                             TIMELINE_RADIUS);

    ImGui::PushID(-1);
    ImGui::SetCursorScreenPos(start);
    ImGui::InvisibleButton(str_id, end - start);
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
        values[0] += ImGui::GetIO().MouseDelta.x / win->Size.x * s_max_timeline_value;
        values[1] += ImGui::GetIO().MouseDelta.x / win->Size.x * s_max_timeline_value;
        changed = true;
    }
    ImGui::PopID();

    ImGui::SetCursorScreenPos(cursor_pos + ImVec2(0, ImGui::GetTextLineHeightWithSpacing()));

    win->DrawList->AddRectFilled(start, end, ImGui::IsItemActive() || ImGui::IsItemHovered() ? active_color : inactive_color);
    
    if (values[0] > values[1]) {
        float tmp = values[0];
        values[0] = values[1];
        values[1] = tmp;
    }
    if (values[1] > s_max_timeline_value) values[1] = s_max_timeline_value;
    if (values[0] < 0) values[0] = 0;
    return changed;
}


void EndTimeline() {
    ImGuiWindow* win = ImGui::GetCurrentWindow();

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    
    ImU32 color = ImGui::ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_Button]);
    ImU32 line_color = ImGui::ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_Border]);
    ImU32 text_color = ImGui::ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_Text]);
    float rounding = GImGui->Style.ScrollbarRounding;
    ImVec2 start(ImGui::GetWindowContentRegionMin().x + win->Pos.x,
        ImGui::GetWindowContentRegionMax().y - ImGui::GetTextLineHeightWithSpacing() + win->Pos.y);
    ImVec2 end = ImGui::GetWindowContentRegionMax() + win->Pos;

    win->DrawList->AddRectFilled(start, end, color, rounding);

    const int LINE_COUNT = 5;
    const ImVec2 text_offset(0, ImGui::GetTextLineHeightWithSpacing());
    for (int i = 0; i <= LINE_COUNT; ++i) {
        ImVec2 a = ImGui::GetWindowContentRegionMin() + win->Pos + ImVec2(TIMELINE_RADIUS, 0);
        a.x += i * ((ImGui::GetWindowContentRegionMax()-ImGui::GetWindowContentRegionMin()).x - 2*style.WindowPadding.x) / LINE_COUNT;
        ImVec2 b = a;
        b.y = start.y;
        win->DrawList->AddLine(a, b, line_color);
        if(i == LINE_COUNT) break;
        char tmp[256];
        ImFormatString(tmp, sizeof(tmp), "%lld", (long long)(i * s_max_timeline_value / LINE_COUNT));
        win->DrawList->AddText(b, text_color, tmp);
    }

    ImGui::EndChild();
}

void ImGuiWidgetFlameGraph::PlotFlame(const char* overlayText,
                                      ImU16 minDepth,
                                      int valuesCount,
                                      ValuesGetter valuesGetter,
                                      OnClick onClick,
                                      ResetFocus resetFocus,
                                      PushFocus pushFocus,
                                      PopFocus popFocus,
                                      GetStackSize getStackSize,
                                      void* data) {

    ImVec2 graph_size = ImVec2(0, 0);
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    

    // Find the maximum depth
    ImU16 maxDepth = minDepth;
    for (int i = 0; i < valuesCount; ++i) {
        ImU16 depth;
        valuesGetter(nullptr, nullptr, &depth, nullptr, data, i);
        maxDepth = ImMax(maxDepth, depth);
    }

    const ImVec2 text_size = ImGui::CalcTextSize(overlayText, NULL, true);
    const auto blockHeight = ImGui::GetTextLineHeight() + (style.FramePadding.y * 2);
    if (graph_size.x == 0.0f)
        graph_size.x = ImGui::GetWindowSize().x - 4*style.FramePadding.x - window->ScrollbarY * style.ScrollbarSize;
    if (graph_size.y == 0.0f)
        graph_size.y = text_size.y + (style.FramePadding.y * 3) + blockHeight * (maxDepth + 1);

    const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + graph_size);
    const ImRect inner_bb(frame_bb.Min + style.FramePadding, frame_bb.Max - style.FramePadding);
    ImGui::ItemSize(frame_bb, style.FramePadding.y);
    if (!ImGui::ItemAdd(frame_bb, 0, &frame_bb))
        return;

    // Determine scale from values if not specified
    float v_min = FLT_MAX;
    float v_max = -FLT_MAX;
    for (int i = 0; i < valuesCount; i++) {
        float v_start, v_end;
        valuesGetter(&v_start, &v_end, nullptr, nullptr, data, i);
        if (v_start == v_start) // Check non-NaN values
            v_min = ImMin(v_min, v_start);
        if (v_end == v_end) // Check non-NaN values
            v_max = ImMax(v_max, v_end);
    }
    float scale_min = v_min;
    float scale_max = v_max;

    ImGui::RenderFrame(frame_bb.Min, frame_bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);

    bool any_hovered = false;
    if (valuesCount>= 1) {
        const ImU32 col_base = ImGui::GetColorU32(ImGuiCol_PlotHistogram) & 0x77FFFFFF;
        const ImU32 col_hovered = ImGui::GetColorU32(ImGuiCol_PlotHistogramHovered) & 0x77FFFFFF;
        const ImU32 col_outline_base = ImGui::GetColorU32(ImGuiCol_PlotHistogram) & 0x7FFFFFFF;
        const ImU32 col_outline_hovered = ImGui::GetColorU32(ImGuiCol_PlotHistogramHovered) & 0x7FFFFFFF;

        for (int i = 0; i < valuesCount; ++i) {
            float stageStart, stageEnd;
            ImU16 depth;
            const char* caption;

            valuesGetter(&stageStart, &stageEnd, &depth, &caption, data, i);

            auto duration = scale_max - scale_min;
            if (duration == 0) {
                return;
            }

            auto start = stageStart - scale_min;
            auto end = stageEnd - scale_min;

            auto startX = static_cast<float>(start / (double)duration);
            auto endX = static_cast<float>(end / (double)duration);

            float width = inner_bb.Max.x - inner_bb.Min.x;
            float height = blockHeight * (maxDepth - depth + 1) - style.FramePadding.y;

            auto pos0 = inner_bb.Min + ImVec2(startX * width, height);
            auto pos1 = inner_bb.Min + ImVec2(endX * width, height + blockHeight);

            bool v_hovered = false;
            if (ImGui::IsMouseHoveringRect(pos0, pos1)) {
                ImGui::SetTooltip("%s: %8.4g", caption, stageEnd - stageStart);
                v_hovered = true;
                any_hovered = v_hovered;

                if(ImGui::IsItemClicked()) {
                    onClick(data, i);
                }
            }

            window->DrawList->AddRectFilled(pos0, pos1, v_hovered ? col_hovered : col_base);
            window->DrawList->AddRect(pos0, pos1, v_hovered ? col_outline_hovered : col_outline_base);
            auto textSize = ImGui::CalcTextSize(caption);
            auto boxSize = (pos1 - pos0);
            if (textSize.x < boxSize.x) {
                auto textOffset = ImVec2(0.5f, 0.5f) * (boxSize - textSize);
                ImGui::RenderText(pos0 + textOffset, caption);
            }
        }

        // Text overlay
        if (overlayText)
            ImGui::RenderTextClipped(ImVec2(frame_bb.Min.x, frame_bb.Min.y + style.FramePadding.y), frame_bb.Max, overlayText, NULL, NULL, ImVec2(0.5f,0.0f));
    }

    if(ImGui::Button("Reset focus")) {
        resetFocus(data);
    }

    ImGui::SameLine();
    if(ImGui::Button("Push focus")) {
        pushFocus(data);
    }

    ImGui::SameLine();
    if(ImGui::Button("Pop focus")) {
        popFocus(data);
    }

    int stackSize = 0;
    getStackSize(data, &stackSize);
    ImGui::SameLine();
    ImGui::Text("Stack size : %d", stackSize);

    if (!any_hovered && ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Total: %8.4g", scale_max - scale_min);
    }
}
