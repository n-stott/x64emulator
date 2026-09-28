cmake_minimum_required(VERSION 3.14)
project(x64)
set(CMAKE_CXX_STANDARD 17)

option(MULTIPROCESSING "MULTIPROCESSING" OFF)
option(SSE3 "SSE3" OFF)
option(SSSE3 "SSSE3" OFF)
option(SSE41 "SSE41" OFF)
option(SSE42 "SSE42" OFF)

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
    zydis
    GIT_REPOSITORY         https://github.com/zyantific/zydis.git
    GIT_TAG                v4.1.1
    GIT_SUBMODULES_RECURSE TRUE
)
FetchContent_MakeAvailable(zydis)

FetchContent_Declare(
    capstone
    GIT_REPOSITORY https://github.com/capstone-engine/capstone.git
    GIT_TAG        5.0
)
FetchContent_MakeAvailable(capstone)

FetchContent_Declare(
    boost_int128
    GIT_REPOSITORY https://github.com/cppalliance/int128.git
    GIT_TAG        v1.2.0
)
FetchContent_MakeAvailable(boost_int128)

FetchContent_Declare(
    boost_config
    GIT_REPOSITORY https://github.com/boostorg/config.git
    GIT_TAG boost-1.74.0
)
FetchContent_Declare(
    boost_assert
    GIT_REPOSITORY https://github.com/boostorg/assert.git
    GIT_TAG boost-1.74.0
)
FetchContent_Declare(
    boost_core
    GIT_REPOSITORY https://github.com/boostorg/core.git
    GIT_TAG boost-1.74.0
)
FetchContent_Declare(
    boost_preprocessor
    GIT_REPOSITORY https://github.com/boostorg/preprocessor.git
    GIT_TAG boost-1.74.0
)
FetchContent_Declare(
    boost_static_assert
    GIT_REPOSITORY https://github.com/boostorg/static_assert.git
    GIT_TAG boost-1.74.0
)
FetchContent_Declare(
    boost_type_traits
    GIT_REPOSITORY https://github.com/boostorg/type_traits.git
    GIT_TAG boost-1.74.0
)
FetchContent_Declare(
    boost_throw_exception
    GIT_REPOSITORY https://github.com/boostorg/throw_exception.git
    GIT_TAG boost-1.74.0
)
FetchContent_Declare(
    boost_integer
    GIT_REPOSITORY https://github.com/boostorg/integer.git
    GIT_TAG boost-1.74.0
)
FetchContent_Declare(
    boost_move
    GIT_REPOSITORY https://github.com/boostorg/move.git
    GIT_TAG boost-1.74.0
)
FetchContent_Declare(
    boost_detail
    GIT_REPOSITORY https://github.com/boostorg/detail.git
    GIT_TAG boost-1.74.0
)
FetchContent_Declare(
    boost_intrusive
    GIT_REPOSITORY https://github.com/boostorg/intrusive.git
    GIT_TAG boost-1.74.0
)
FetchContent_Declare(
    boost_container_hash
    GIT_REPOSITORY https://github.com/boostorg/container_hash.git
    GIT_TAG boost-1.74.0
)
FetchContent_Declare(
    boost_container
    GIT_REPOSITORY https://github.com/boostorg/container.git
    GIT_TAG boost-1.74.0
)
FetchContent_MakeAvailable(
    boost_config
    boost_assert
    boost_core
    boost_preprocessor
    boost_static_assert
    boost_type_traits
    boost_throw_exception
    boost_integer
    boost_move
    boost_detail
    boost_intrusive
    boost_container_hash
    boost_container
)

FetchContent_Declare(
        SDL2
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-2.26.5
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(SDL2)

if(${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL x86_64)
    add_compile_definitions(ARCH_X64=1)
    set(EMULATOR_ARCH X64)
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL aarch64)
    add_compile_definitions(ARCH_ARM64=1)
    set(EMULATOR_ARCH ARM64)
else()
    message(FATAL_ERROR "Unsupported architecture")
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL Linux)
    add_compile_definitions(HOST_LINUX=1)
    set(EMULATOR_HOST LINUX)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL Windows)
    add_compile_definitions(HOST_WINDOWS=1)
    set(EMULATOR_HOST WINDOWS)
else()
    message(FATAL_ERROR "Unsupported system")
endif()

if(${CMAKE_CXX_COMPILER_ID} STREQUAL GNU)
    add_compile_definitions(GCC_COMPILER=1)
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
    add_compile_definitions(CLANG_COMPILER=1)
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
    add_compile_definitions(MSVC_COMPILER=1)
else()
    message(FATAL_ERROR "Unsupported compiler")
endif()

add_subdirectory(profiling)
add_subdirectory(emulator)
add_subdirectory(profileviewer)

enable_testing()
add_subdirectory(tests)