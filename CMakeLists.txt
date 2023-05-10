cmake_minimum_required(VERSION 3.13)
project(elf-reader)
set(CMAKE_CXX_STANDARD 17)

set(SOURCE_FILES
    src/elf-reader.cpp
    src/main.cpp
)

add_executable(elf-reader ${SOURCE_FILES})

target_include_directories(elf-reader PUBLIC external)
target_compile_options(elf-reader PUBLIC -g)
target_link_libraries(elf-reader ${CMAKE_SOURCE_DIR}/external/fmt/libfmt.a)
