#include "profilingdata.h"
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>
#include <imgui_widget_flamegraph.h>
#include <fmt/core.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include <cassert>
#include <fstream>
#include <stack>
#include <unordered_map>

struct ProfileRange {
    u64 start;
    u64 end;
    u32 symbolIndex;
    u32 depth;
};

struct AllProfileData {
    std::vector<ProfileRange> profileRanges;
    std::vector<std::string> symbols;
};

int main(int argc, char** argv) {
    using namespace profiling;
    
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

    if(profileData->nbThreads() == 0) {
        fmt::print(stderr, "Cannot show profile data for 0 threads\n");
        return 1;
    }

    AllProfileData allProfileData;
    const ThreadProfilingData& tpd = profileData->threadData(0);

    std::vector<ThreadProfilingData::CallEvent> callEvents;
    callEvents.reserve(tpd.nbCallEvents());
    tpd.forEachCallEvent([&](const auto& e) { callEvents.push_back(e); });

    std::vector<ThreadProfilingData::RetEvent> retEvents;
    retEvents.reserve(tpd.nbRetEvents());
    tpd.forEachRetEvent([&](const auto& e) { retEvents.push_back(e); });

    auto callIt = callEvents.cbegin();
    auto retIt = retEvents.cbegin();
    auto callEnd = callEvents.cend();
    auto retEnd = retEvents.cend();
    
    std::unordered_map<u64, u32> addressToSymbolIndex;
    allProfileData.symbols.push_back("???");
    u32 unknownSymbolIndex = 0;

    u64 maxTick = 0;
    std::stack<ProfileRange> stack;
    for(; callIt != callEnd && retIt != retEnd;) {
        assert(callIt->tick != retIt->tick);
        if(callIt->tick < retIt->tick) {
            u32 symbolIndex = unknownSymbolIndex;
            auto symbolIndexIt = addressToSymbolIndex.find(callIt->address);
            if(symbolIndexIt == addressToSymbolIndex.end()) {
                auto symbol = profileData->symbolTable().findSymbol(callIt->address);
                if(!!symbol) {
                    symbolIndex = (u32)allProfileData.symbols.size();
                    addressToSymbolIndex[callIt->address] = symbolIndex;
                    allProfileData.symbols.push_back(symbol.value());
                } else {
                    addressToSymbolIndex[callIt->address] = unknownSymbolIndex;
                }
            } else {
                symbolIndex = symbolIndexIt->second;
            }

            stack.push(ProfileRange {
                callIt->tick,
                (u64)(-1), // filled in later
                symbolIndex,
                (u32)stack.size(),
            });
            maxTick = std::max(maxTick, callIt->tick);
            ++callIt;
        } else {
            ProfileRange range = stack.top();
            stack.pop();
            range.end = retIt->tick;
            maxTick = std::max(maxTick, retIt->tick);
            allProfileData.profileRanges.push_back(range);
            ++retIt;
        }
    }

    fmt::print("{} ranges remaining in stack\n", stack.size());
    while(!stack.empty()) {
            ProfileRange range = stack.top();
            stack.pop();
            range.end = maxTick+1;
            allProfileData.profileRanges.push_back(range);
    }
    fmt::print("Created {} profile ranges\n", allProfileData.profileRanges.size());
    
    if(SDL_Init(SDL_INIT_EVERYTHING) != 0) return 1;


    const std::string windowTitle = "profileviewer";
    const int windowX = SDL_WINDOWPOS_CENTERED;
    const int windowY = SDL_WINDOWPOS_CENTERED;
    const int windowW = 1200;
    const int windowH = 600;
    const u32 windowFlags = 0;
    SDL_Window* window = SDL_CreateWindow(windowTitle.c_str(), windowX, windowY, windowW, windowH, windowFlags);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    const char* glsl_version = "#version 130";
    ImGui_ImplOpenGL3_Init(glsl_version);

    ImGuiIO& io = ImGui::GetIO();

    bool done = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

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
        auto valuesGetter = [](float* start, float* end, ImU8* level, const char** caption, const void* data, int idx) {
            const AllProfileData* profileData = (const AllProfileData*)data;
            const ProfileRange& range = profileData->profileRanges[idx];
            if(!!start) *start = (float)range.start;
            if(!!end) *end = (float)range.end;
            if(!!level) *level = (ImU8)range.depth;
            if(!!caption) *caption = profileData->symbols[range.symbolIndex].c_str();
        };
        const void* data = &allProfileData;
        int values_count = (int)allProfileData.profileRanges.size();
        ImGuiWidgetFlameGraph::PlotFlame(label.c_str(), valuesGetter, data, values_count);

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