cmake_minimum_required(VERSION 3.14)
project(x64)
set(CMAKE_CXX_STANDARD 17)

add_executable(profileviewer
    src/main.cpp
)
target_compile_options(profileviewer PUBLIC -Wall -Wextra -Wpedantic -pedantic -Wconversion -Werror -Wimplicit-fallthrough)
target_compile_options(profileviewer PUBLIC -Wno-braced-scalar-init -Wno-missing-braces)
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    target_compile_options(profileviewer PUBLIC -Wno-maybe-uninitialized -Wno-stringop-overflow)
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    target_compile_options(profileviewer PUBLIC -Wno-uninitialized)
endif()
target_compile_options(profileviewer PUBLIC
    -DFMT_HEADER_ONLY
    -DFMT_USE_FLOAT=0
    -DFMT_USE_DOUBLE=0
    -DFMT_USE_LONG_DOUBLE=0
)
target_include_directories(profileviewer PUBLIC include)
target_include_directories(profileviewer PUBLIC ${CMAKE_SOURCE_DIR}/profiling/include)
target_link_libraries(profileviewer PUBLIC
    profiling
    fmt::fmt
    nlohmann_json::nlohmann_json
)