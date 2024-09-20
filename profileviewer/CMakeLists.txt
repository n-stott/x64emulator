cmake_minimum_required(VERSION 3.14)
project(x64)
set(CMAKE_CXX_STANDARD 17)

add_executable(profileviewer
    external/imgui/imgui.cpp
    external/imgui/imgui_demo.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/backends/imgui_impl_sdl2.cpp
    external/imgui/backends/imgui_impl_opengl3.cpp
    src/main.cpp
)
target_compile_options(profileviewer PUBLIC -Wall -Wextra -Wpedantic -pedantic -Werror -Wimplicit-fallthrough)
target_compile_options(profileviewer PUBLIC -Wno-braced-scalar-init -Wno-missing-braces)
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    target_compile_options(profileviewer PUBLIC -Wno-maybe-uninitialized -Wno-stringop-overflow)
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    target_compile_options(profileviewer PUBLIC -Wno-uninitialized)
endif()
target_include_directories(profileviewer PUBLIC
    include
    ${CMAKE_SOURCE_DIR}/profiling/include
    external/imgui
    external/imgui/backends
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
