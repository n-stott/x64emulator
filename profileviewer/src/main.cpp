#include "profilingdata.h"
#include "range.h"
#include "profiledata.h"
#include "focusedprofiledata.h"
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
            fmt::print("Thread {}:{} : {} call events, {} ret events and {} sys events.\n", tpd.pid(), tpd.tid(), tpd.nbCallEvents(), tpd.nbRetEvents(), tpd.nbSyscallEvents());
        }
    }

    auto allProfileData = AllProfileData::tryCreate(*profileData);

    if(!allProfileData) {
        fmt::print("Unable to extract profile data\n");
        return 1;
    }

    FocusedProfileData focusedProfileData(*allProfileData);
    
    if(SDL_Init(SDL_INIT_EVERYTHING) != 0) return 1;


    const std::string windowTitle = "profileviewer";
    const int windowX = SDL_WINDOWPOS_CENTERED;
    const int windowY = SDL_WINDOWPOS_CENTERED;
    const int windowW = 1200;
    const int windowH = 600;
    const SDL_WindowFlags windowFlags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow(windowTitle.c_str(), windowX, windowY, windowW, windowH, windowFlags);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    focusedProfileData.setMergeThreshold(1.0/windowW);

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    const char* glsl_version = "#version 130";
    ImGui_ImplOpenGL3_Init(glsl_version);

    bool done = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    Range wholeRange = focusedProfileData.focusedRange();
    float bounds[2] = { 0.0, (float)wholeRange.width() };

    focusedProfileData.addNewFocusRangeCallback([&](Range newFocusRange) {
        bounds[0] = (float)newFocusRange.begin;
        bounds[1] = (float)newFocusRange.end; 
    });

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

        bool showDialog { true };
        ImGui::SetNextWindowSize(ImVec2(windowW, windowH));
        ImGui::SetNextWindowPos(ImVec2(0, 0)); // Set the position of the new window
        ImGui::Begin("Profile", &showDialog, ImGuiWindowFlags_NoCollapse);

        std::string label = "Thread 0";
        auto valuesGetter = [](float* start, float* end, ImU16* level, const char** caption, const void* data, int idx) {
            const FocusedProfileData* focusedProfileData = (const FocusedProfileData*)data;
            const ProfileRange& pr = focusedProfileData->focusedProfileRanges()[idx];
            if(!!start) *start = (float)pr.range.begin;
            if(!!end) *end = (float)pr.range.end;
            if(!!level) *level = (ImU16)pr.depth;
            if(!!caption) *caption = focusedProfileData->data().symbols[pr.symbolIndex].c_str();
        };

        auto onClick = [](void* data, int idx) {
            FocusedProfileData* focusedProfileData = (FocusedProfileData*)data;
            const ProfileRange& pr = focusedProfileData->focusedProfileRanges()[idx];
            focusedProfileData->setFocusRange(pr.range);
            focusedProfileData->push();
        };

        auto resetFocus = [](void* data) {
            FocusedProfileData* focusedProfileData = (FocusedProfileData*)data;
            focusedProfileData->reset();
        };

        auto pushFocus = [](void* data) {
            FocusedProfileData* focusedProfileData = (FocusedProfileData*)data;
            focusedProfileData->push();
        };

        auto popFocus = [](void* data) {
            FocusedProfileData* focusedProfileData = (FocusedProfileData*)data;
            focusedProfileData->pop();
        };
        
        auto getStackSize = [](void* data, int* stackSize) {
            FocusedProfileData* focusedProfileData = (FocusedProfileData*)data;
            if(!!stackSize) *stackSize = (int)focusedProfileData->stackSize();
        };

        void* data = &focusedProfileData;
        int valuesCount = (int)focusedProfileData.focusedProfileRanges().size();
        ImGuiWidgetFlameGraph::PlotFlame(label.c_str(),
                                         allProfileData->maxDepth,
                                         valuesCount,
                                         valuesGetter,
                                         onClick,
                                         resetFocus,
                                         pushFocus,
                                         popFocus,
                                         getStackSize,
                                         data);


        BeginTimeline("timeline", (float)wholeRange.width());
        if(TimelineEvent("timeline", bounds)) {
            focusedProfileData.setFocusRange(Range{
                (u64)bounds[0],
                (u64)bounds[1],
            });
        }
        EndTimeline();

        ImGui::End();

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
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