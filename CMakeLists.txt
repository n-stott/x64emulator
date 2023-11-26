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

add_library(emulator STATIC
    src/lib/library.cpp
    src/lib/libc.cpp
    src/lib/heap.cpp
    src/translation/x86instruction.cpp
    src/disassembler/capstonewrapper.cpp
    src/interpreter/loader.cpp
    src/interpreter/symbolprovider.cpp
    src/interpreter/interpreter.cpp
    src/interpreter/cpu.cpp
    src/interpreter/mmu.cpp
    src/interpreter/syscalls.cpp
    src/interpreter/executioncontext.cpp
    src/program.cpp
)
target_compile_options(emulator PUBLIC -Wall -Wextra -Wpedantic -pedantic -Wconversion -Werror)
target_compile_options(emulator PUBLIC -Wno-braced-scalar-init -Wimplicit-fallthrough -Wno-missing-braces)
target_compile_options(emulator PUBLIC -DFMT_HEADER_ONLY)
target_include_directories(emulator PUBLIC include)
target_include_directories(emulator PUBLIC ../common/include)
target_include_directories(emulator PUBLIC external)
target_include_directories(emulator PUBLIC external/elf-reader/src)
target_link_libraries(emulator PUBLIC -lstdc++fs -lcapstone)

add_executable(interpreter
    src/main.cpp
)
target_compile_options(interpreter PUBLIC -Wall -Wextra -Wpedantic -pedantic -Wconversion -Werror)
target_compile_options(interpreter PUBLIC -Wno-braced-scalar-init -Wimplicit-fallthrough -Wno-missing-braces)
target_include_directories(interpreter PUBLIC include)
target_include_directories(interpreter PUBLIC ../common/include)
target_include_directories(interpreter PUBLIC external)
target_link_libraries(interpreter PUBLIC emulator)

add_subdirectory(tests)