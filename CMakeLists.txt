cmake_minimum_required(VERSION 3.13)
project(x64)
set(CMAKE_CXX_STANDARD 17)

enable_testing()

add_executable(elf-reader
    src/elf-reader/main.cpp
)
target_include_directories(elf-reader PUBLIC external)
target_compile_options(elf-reader PUBLIC -Wall -Wextra -Wpedantic -pedantic -Wconversion -Werror)
target_compile_options(elf-reader PUBLIC -g)
target_compile_options(elf-reader PUBLIC -DFMT_HEADER_ONLY)
target_include_directories(elf-reader PUBLIC include)


add_library(x64 STATIC
    src/interpreter/cpuimpl.cpp
    src/interpreter/x87.cpp
    src/utils/host.cpp
    src/utils/utils.cpp
)
target_compile_options(x64 PUBLIC -Wall -Wextra -Wpedantic -pedantic -Wconversion -Werror -Wimplicit-fallthrough)
target_compile_options(x64 PUBLIC -Wno-braced-scalar-init -Wno-missing-braces -Wno-maybe-uninitialized)
target_compile_options(x64 PUBLIC -frounding-math)
target_include_directories(x64 PUBLIC include)


add_library(emulator STATIC
    src/disassembler/capstonewrapper.cpp
    src/interpreter/auxiliaryvector.cpp
    src/interpreter/symbolprovider.cpp
    src/interpreter/interpreter.cpp
    src/interpreter/vm.cpp
    src/interpreter/cpu.cpp
    src/interpreter/cpuimpl.cpp
    src/interpreter/checkedcpuimpl.cpp
    src/interpreter/mmu.cpp
    src/interpreter/x87.cpp
    src/interpreter/syscalls.cpp
    src/instructions/x64instruction.cpp
    src/utils/host.cpp
    src/utils/utils.cpp
)
target_compile_options(emulator PUBLIC -Wall -Wextra -Wpedantic -pedantic -Wconversion -Werror -Wimplicit-fallthrough)
target_compile_options(emulator PUBLIC -Wno-braced-scalar-init -Wno-missing-braces -Wno-maybe-uninitialized)
target_compile_options(emulator PUBLIC -frounding-math)
target_compile_options(emulator PUBLIC -DFMT_HEADER_ONLY)
target_include_directories(emulator PUBLIC include)
target_include_directories(emulator PUBLIC external)
target_include_directories(emulator PUBLIC external/elf-reader/src)
target_link_libraries(emulator PUBLIC -lcapstone)


add_executable(interpreter
    src/main.cpp
)
target_compile_options(interpreter PUBLIC -Wall -Wextra -Wpedantic -pedantic -Wconversion -Werror -Wimplicit-fallthrough)
target_compile_options(interpreter PUBLIC -Wno-braced-scalar-init -Wno-missing-braces -Wno-maybe-uninitialized)
target_include_directories(interpreter PUBLIC include)
target_include_directories(interpreter PUBLIC ../common/include)
target_include_directories(interpreter PUBLIC external)
target_link_libraries(interpreter PUBLIC emulator)

add_subdirectory(tests)