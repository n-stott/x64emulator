cmake_minimum_required(VERSION 3.13)
project(x64)
set(CMAKE_CXX_STANDARD 17)

enable_testing()

add_library(emulator STATIC
    src/disassembler/capstonewrapper.cpp
    src/interpreter/loader.cpp
    src/interpreter/symbolprovider.cpp
    src/interpreter/interpreter.cpp
    src/interpreter/vm.cpp
    src/interpreter/cpu.cpp
    src/interpreter/mmu.cpp
    src/interpreter/syscalls.cpp
)
target_compile_options(emulator PUBLIC -Wall -Wextra -Wpedantic -pedantic -Wconversion -Werror)
target_compile_options(emulator PUBLIC -Wno-braced-scalar-init -Wimplicit-fallthrough -Wno-missing-braces)
target_compile_options(emulator PUBLIC -DFMT_HEADER_ONLY)
target_include_directories(emulator PUBLIC include)
target_include_directories(emulator PUBLIC ../common/include)
target_include_directories(emulator PUBLIC external)
target_include_directories(emulator PUBLIC external/elf-reader/src)
target_link_libraries(emulator PUBLIC -lcapstone)

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