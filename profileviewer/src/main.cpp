#include "profilingdata.h"
#include "range.h"
#include "profiledata.h"
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>
#include <imgui_widget_flamegraph.h>
#include <fmt/core.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include <fstream>
#include <unordered_map>

int main(int argc, char** argv) {
    using namespace profiling;
    using namespace profileviewer;
    
    std::ifstream inputFile(argc != 2 ? "output.json" : argv[1]);
    auto profileData = ProfilingData::tryCreateFromJson(inputFile);
    if(!profileData) {
        fmt::print(stderr, "Unable to read {}\n", "output.json");
        return 1;
    } else {
        fmt::print("Read {} : has data from {} thread(s)\n", "output.json", profileData->nbThreads());
        for(size_t t = 0; t < profileData->nbThreads(); ++t) {
            const ThreadProfilingData& tpd = profileData->threadData(t);
            fmt::print("Thread {}:{} : {} call events and {} ret events.\n", tpd.pid(), tpd.tid(), tpd.nbCallEvents(), tpd.nbRetEvents());
        }
    }

    auto allProfileData = AllProfileData::tryCreate(*profileData);

    if(!allProfileData) {
        fmt::print("Unable to extract profile data\n");
        return 1;
    }

    FocusedProfileData focusedProfileData;
    focusedProfileData.data = allProfileData.get();
    focusedProfileData.focusStack.push(Range{0, allProfileData->maxTick+1});
    focusedProfileData.focusedProfileRanges = allProfileData->profileRanges;
    
    if(SDL_Init(SDL_INIT_EVERYTHING) != 0) return 1;


    const std::string windowTitle = "profileviewer";
    const int windowX = SDL_WINDOWPOS_CENTERED;
    const int windowY = SDL_WINDOWPOS_CENTERED;
    const int windowW = 1200;
    const int windowH = 600;
    const SDL_WindowFlags windowFlags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow(windowTitle.c_str(), windowX, windowY, windowW, windowH, windowFlags);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);


    ImGui::CreateContext();
    auto& imGuiIO = ImGui::GetIO();
    imGuiIO.IniFilename = nullptr;
    imGuiIO.LogFilename = nullptr;
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    const char* glsl_version = "#version 130";
    ImGui_ImplOpenGL3_Init(glsl_version);

    ImGuiIO& io = ImGui::GetIO();

    bool done = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    assert(!focusedProfileData.focusedProfileRanges.empty());
    ProfileRange first = focusedProfileData.focusedProfileRanges.front();
    ProfileRange last = focusedProfileData.focusedProfileRanges.back();
    Range wholeRange{first.range.begin, last.range.end};
    fmt::print("initial focus range : {}-{}\n", wholeRange.begin, wholeRange.end);
    float bounds[2] = { 0.0, (float)wholeRange.width() };

    while (!done) {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }
        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)
        {
            SDL_Delay(10);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        std::string label = "calls";
        auto valuesGetter = [](float* start, float* end, ImU16* level, const char** caption, const void* data, int idx) {
            const FocusedProfileData* focusedProfileData = (const FocusedProfileData*)data;
            const ProfileRange& pr = focusedProfileData->focusedProfileRanges[idx];
            if(!!start) *start = (float)pr.range.begin;
            if(!!end) *end = (float)pr.range.end;
            if(!!level) *level = (ImU16)pr.depth;
            if(!!caption) *caption = focusedProfileData->data->symbols[pr.symbolIndex].c_str();
        };

        auto onClick = [](void* data, int idx) {
            FocusedProfileData* focusedProfileData = (FocusedProfileData*)data;
            const ProfileRange& pr = focusedProfileData->focusedProfileRanges[idx];
            focusedProfileData->newFocusRange = pr.range;
        };

        auto popFocusStack = [](void* data) {
            FocusedProfileData* focusedProfileData = (FocusedProfileData*)data;
            if(focusedProfileData->focusStack.size() <= 1) return;
            focusedProfileData->focusStack.pop();
            focusedProfileData->newFocusRange = focusedProfileData->focusStack.top();
            focusedProfileData->focusStack.pop();
            fmt::print("stack has {} elements remaining\n", focusedProfileData->focusStack.size());
        };

        void* data = &focusedProfileData;
        int values_offset = 0;
        int values_count = (int)focusedProfileData.focusedProfileRanges.size();
        ImGuiWidgetFlameGraph::PlotFlame(label.c_str(), allProfileData->maxDepth, valuesGetter, onClick, popFocusStack, data, values_count, values_offset);

        if(!!focusedProfileData.newFocusRange) {
            Range newFocusRange = focusedProfileData.newFocusRange.value();
            focusedProfileData.focusStack.push(newFocusRange);
        }

        BeginTimeline("timeline", (float)wholeRange.width());
        if(TimelineEvent("timeline", bounds) && !focusedProfileData.newFocusRange) {
            focusedProfileData.newFocusRange = Range{
                (u64)bounds[0],
                (u64)bounds[1],
            };
        }
        EndTimeline();

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);

        if(!!focusedProfileData.newFocusRange) {
            Range newFocusRange = focusedProfileData.newFocusRange.value();
            focusedProfileData.newFocusRange.reset();

            ProfileRange fakeFocusProfileRangeStart;
            fakeFocusProfileRangeStart.range.begin = newFocusRange.begin;
            fakeFocusProfileRangeStart.range.end = newFocusRange.begin;
            auto begin = std::lower_bound(allProfileData->profileRanges.begin(),
                                          allProfileData->profileRanges.end(),
                                          fakeFocusProfileRangeStart);

            ProfileRange fakeFocusProfileRangeEnd;
            fakeFocusProfileRangeEnd.range.begin = newFocusRange.end;
            fakeFocusProfileRangeEnd.range.end = newFocusRange.end;
            auto end = std::upper_bound(allProfileData->profileRanges.begin(),
                                        allProfileData->profileRanges.end(),
                                        fakeFocusProfileRangeEnd);

            focusedProfileData.focusedProfileRanges = std::vector<ProfileRange>(begin, end);
            std::for_each(focusedProfileData.focusedProfileRanges.begin(),
                          focusedProfileData.focusedProfileRanges.end(), [&](ProfileRange& pr) {
                pr.range = Range::intersection(pr.range, newFocusRange);
            });

            for(auto it = allProfileData->profileRanges.begin(); it != begin; ++it) {
                if(newFocusRange.intersects(it->range)) {
                    ProfileRange clamped = *it;
                    clamped.range = Range::intersection(clamped.range, newFocusRange);
                    focusedProfileData.focusedProfileRanges.push_back(clamped);
                }
            }

            bounds[0] = (float)newFocusRange.begin;
            bounds[1] = (float)newFocusRange.end;
        }
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}