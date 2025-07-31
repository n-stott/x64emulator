cmake_minimum_required(VERSION 3.14)
project(x64)
set(CMAKE_CXX_STANDARD 17)

add_library(profiling STATIC
    src/profilingdata.cpp
)
target_compile_options(profiling PUBLIC -Wall -Wextra -Wpedantic -pedantic -Werror)
target_include_directories(profiling PUBLIC include)
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    target_compile_options(profiling PUBLIC -Wno-maybe-uninitialized -Wno-stringop-overflow)
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    target_compile_options(profiling PUBLIC -Wno-uninitialized)
endif()
target_link_libraries(profiling
    fmt::fmt-header-only
    nlohmann_json::nlohmann_json
)