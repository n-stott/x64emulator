cmake_minimum_required(VERSION 3.14)
project(x64)
set(CMAKE_CXX_STANDARD 17)

option(MULTIPROCESSING "MULTIPROCESSING" OFF)
option(SSE3 "SSE3" OFF)
option(SSSE3 "SSSE3" OFF)
option(SSE41 "SSE41" OFF)

include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG        11.0.2
)
FetchContent_MakeAvailable(fmt)

FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(json)

FetchContent_Declare(
    argparse
    GIT_REPOSITORY https://github.com/p-ranav/argparse.git
)
FetchContent_MakeAvailable(argparse)

FetchContent_Declare(
    capstone
    GIT_REPOSITORY https://github.com/capstone-engine/capstone.git
    GIT_TAG        4.0
)
set(CAPSTONE_BUILD_TESTS OFF CACHE BOOL "Disable capstone tests" FORCE)
FetchContent_MakeAvailable(capstone)

FetchContent_Declare(
    zydis
    GIT_REPOSITORY         https://github.com/zyantific/zydis.git
    GIT_TAG                v4.1.1
    GIT_SUBMODULES_RECURSE TRUE
)
FetchContent_MakeAvailable(zydis)

FetchContent_Declare(
    boost_int128
    GIT_REPOSITORY https://github.com/cppalliance/int128.git
    GIT_TAG        v1.2.0
)
FetchContent_MakeAvailable(boost_int128)

if(${CMAKE_CXX_COMPILER_ID} STREQUAL GNU)
    add_compile_definitions(GCC_COMPILER=1)
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
    add_compile_definitions(CLANG_COMPILER=1)
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
    add_compile_definitions(MSVC_COMPILER=1)
endif()

add_subdirectory(profiling)
add_subdirectory(emulator)
add_subdirectory(profileviewer)

enable_testing()
add_subdirectory(tests)