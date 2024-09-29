cmake_minimum_required(VERSION 3.14)
project(x64)
set(CMAKE_CXX_STANDARD 17)

add_library(profiling
    src/profilingdata.cpp
)
target_compile_options(profiling PUBLIC -Wall -Wextra -Wpedantic -pedantic -Werror)
target_include_directories(profiling PUBLIC include)
target_link_libraries(profiling
    fmt::fmt-header-only
    nlohmann_json::nlohmann_json
)