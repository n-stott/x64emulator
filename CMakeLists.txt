cmake_minimum_required(VERSION 3.14)
project(x64)
set(CMAKE_CXX_STANDARD 17)

option(MULTIPROCESSING "MULTIPROCESSING" OFF)

include(FindPkgConfig)
pkg_check_modules (CAPSTONE REQUIRED capstone)


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

add_subdirectory(profiling)
add_subdirectory(emulator)
add_subdirectory(profileviewer)

enable_testing()
add_subdirectory(tests)