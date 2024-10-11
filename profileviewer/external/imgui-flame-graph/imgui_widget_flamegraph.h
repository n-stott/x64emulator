// https://github.com/bwrsandman/imgui-flame-graph
// v 1.00
//
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

// History :
// 2019-10-22 First version

#pragma once

#include <climits>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>

namespace ImGuiWidgetFlameGraph {
    using ValuesGetter = void (*)(float* start, float* end, ImU16* level, const char** caption, const void* data, int idx);
    using OnClick = void(*)(void* data, int idx);
    using ResetFocus = void(*)(void* data);
    using PushFocus = void(*)(void* data);
    using PopFocus = void(*)(void* data);
    using GetStackSize = void(*)(void* data, int*);
    IMGUI_API void PlotFlame(const char* overlayText,
                             ImU16 minDepth,
                             int valuesCount,
                             ValuesGetter valuesGetter,
                             OnClick onClick,
                             ResetFocus resetFocus,
                             PushFocus pushFocus,
                             PopFocus popFocus,
                             GetStackSize getStackSize,
                             void* data);
}

bool BeginTimeline(const char* str_id, float max_value);
bool TimelineEvent(const char* str_id, float* values);
void EndTimeline();