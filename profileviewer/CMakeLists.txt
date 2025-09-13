cmake_minimum_required(VERSION 3.14)
project(x64)
set(CMAKE_CXX_STANDARD 17)

if(${CMAKE_CXX_COMPILER_ID} STREQUAL GNU OR ${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
    add_executable(profileviewer
        external/imgui/imgui.cpp
        external/imgui/imgui_demo.cpp
        external/imgui/imgui_draw.cpp
        external/imgui/imgui_tables.cpp
        external/imgui/imgui_widgets.cpp
        external/imgui/backends/imgui_impl_sdl2.cpp
        external/imgui/backends/imgui_impl_opengl3.cpp
        external/imgui-flame-graph/imgui_widget_flamegraph.cpp
        src/focusedprofiledata.cpp
        src/profiledata.cpp
        src/main.cpp
    )
    target_compile_options(profileviewer PUBLIC -Wall -Wextra -Wpedantic -pedantic -Werror)
    target_compile_options(profileviewer PUBLIC -Wno-braced-scalar-init -Wno-missing-braces)
    target_compile_options(profileviewer PUBLIC -Wno-maybe-uninitialized -Wno-stringop-overflow -Wimplicit-fallthrough)
    target_include_directories(profileviewer PUBLIC
        include
        ${CMAKE_SOURCE_DIR}/profiling/include
        external/imgui
        external/imgui/backends
        external/imgui-flame-graph
        /usr/include/SDL2
    )
    target_link_libraries(profileviewer PUBLIC
        profiling
        fmt::fmt-header-only
        nlohmann_json::nlohmann_json
        pthread
        SDL2
        dl
        GL
    )
endif()