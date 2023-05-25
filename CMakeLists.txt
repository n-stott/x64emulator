cmake_minimum_required(VERSION 3.13)
project(x64)
set(CMAKE_CXX_STANDARD 17)

enable_testing()

add_library(fakelibc SHARED
    src/lib/fakelibc.cpp
)
target_compile_options(fakelibc PUBLIC -fPIC -O0)
target_link_options(fakelibc PUBLIC)
target_include_directories(fakelibc PUBLIC external)
target_include_directories(fakelibc PUBLIC include)

add_executable(interpreter
    external/elf-reader/src/elf-reader.cpp
    src/lib/library.cpp
    src/lib/libc.cpp
    src/translation/x86instruction.cpp
    src/disassembler/capstonewrapper.cpp
    src/interpreter/interpreter.cpp
    src/interpreter/cpu.cpp
    src/interpreter/fpu.cpp
    src/interpreter/mmu.cpp
    src/interpreter/executioncontext.cpp
    src/program.cpp
    src/main.cpp
)
target_compile_options(interpreter PUBLIC -Wall -Wextra -Wpedantic -pedantic -Wconversion -Werror)
target_compile_options(interpreter PUBLIC -Wno-braced-scalar-init -Wimplicit-fallthrough -Wno-missing-braces)
target_compile_options(interpreter PUBLIC -DFMT_HEADER_ONLY)
target_include_directories(interpreter PUBLIC include)
target_include_directories(interpreter PUBLIC ../common/include)
target_include_directories(interpreter PUBLIC external)
target_include_directories(interpreter PUBLIC external/elf-reader/src)
target_link_libraries(interpreter PUBLIC -lstdc++fs -lcapstone)
# target_compile_options(interpreter PUBLIC -fsanitize=address,undefined)
# target_link_options(interpreter PUBLIC -fsanitize=address,undefined)

add_subdirectory(tests)