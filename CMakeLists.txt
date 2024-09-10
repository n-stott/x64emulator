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
    GIT_TAG        master
)
FetchContent_MakeAvailable(fmt)

add_subdirectory(emulator)

enable_testing()
add_subdirectory(tests)