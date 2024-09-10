cmake_minimum_required(VERSION 3.14)
project(x64)
set(CMAKE_CXX_STANDARD 17)

add_executable(elf-reader
    src/elf-reader/main.cpp
)
target_compile_options(elf-reader PUBLIC -Wall -Wextra -Wpedantic -pedantic -Wconversion -Werror)
target_compile_options(elf-reader PUBLIC -g)
target_include_directories(elf-reader PUBLIC include)
target_link_libraries(elf-reader PUBLIC fmt::fmt)


add_library(x64cpu STATIC
    src/interpreter/checkedcpuimpl.cpp
    src/interpreter/cpuimpl.cpp
    src/interpreter/simd.cpp
    src/interpreter/x87.cpp
    src/host/host.cpp
    src/utils/utils.cpp
)
target_compile_options(x64cpu PUBLIC -Wall -Wextra -Wpedantic -pedantic -Wconversion -Werror -Wimplicit-fallthrough)
target_compile_options(x64cpu PUBLIC -Wno-braced-scalar-init -Wno-missing-braces)
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    target_compile_options(x64cpu PUBLIC -Wno-maybe-uninitialized -Wno-stringop-overflow)
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    target_compile_options(x64cpu PUBLIC -Wno-uninitialized)
endif()
target_compile_options(x64cpu PUBLIC -frounding-math)
target_include_directories(x64cpu PUBLIC include)


add_library(x64emulator STATIC
    src/disassembler/capstonewrapper.cpp
    src/fs/epoll.cpp
    src/fs/event.cpp
    src/fs/fs.cpp
    src/fs/hostfile.cpp
    src/fs/shadowfile.cpp
    src/fs/socket.cpp
    src/fs/stream.cpp
    src/interpreter/auxiliaryvector.cpp
    src/interpreter/symbolprovider.cpp
    src/interpreter/interpreter.cpp
    src/interpreter/kernel.cpp
    src/interpreter/vm.cpp
    src/interpreter/cpu.cpp
    src/interpreter/cpuimpl.cpp
    src/interpreter/checkedcpuimpl.cpp
    src/interpreter/mmu.cpp
    src/interpreter/registers.cpp
    src/interpreter/scheduler.cpp
    src/interpreter/simd.cpp
    src/interpreter/thread.cpp
    src/interpreter/x87.cpp
    src/interpreter/syscalls.cpp
    src/instructions/x64instruction.cpp
    src/host/host.cpp
    src/utils/utils.cpp
)
target_compile_options(x64emulator PUBLIC -Wall -Wextra -Wpedantic -pedantic -Wconversion -Werror -Wimplicit-fallthrough)
target_compile_options(x64emulator PUBLIC -Wno-braced-scalar-init -Wno-missing-braces)
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    target_compile_options(x64emulator PUBLIC -Wno-maybe-uninitialized -Wno-stringop-overflow)
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    target_compile_options(x64emulator PUBLIC -Wno-uninitialized)
endif()
target_compile_options(x64emulator PUBLIC -frounding-math)
target_compile_options(x64emulator PUBLIC -DFMT_HEADER_ONLY)
target_include_directories(x64emulator PUBLIC include)
target_link_libraries(x64emulator PUBLIC ${CAPSTONE_LIBRARIES} fmt::fmt)
if(MULTIPROCESSING)
    target_compile_definitions(x64emulator PUBLIC MULTIPROCESSING=1)
endif(MULTIPROCESSING)

# target_compile_options(x64emulator PUBLIC -fsanitize=address,undefined)
# target_link_libraries(x64emulator PUBLIC asan ubsan)

add_executable(emulator
    src/main.cpp
)
target_compile_options(emulator PUBLIC -Wall -Wextra -Wpedantic -pedantic -Wconversion -Werror -Wimplicit-fallthrough)
target_compile_options(emulator PUBLIC -Wno-braced-scalar-init -Wno-missing-braces)
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    target_compile_options(emulator PUBLIC -Wno-maybe-uninitialized -Wno-stringop-overflow)
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    target_compile_options(emulator PUBLIC -Wno-uninitialized)
endif()
target_include_directories(emulator PUBLIC include)
target_link_libraries(emulator PUBLIC x64emulator fmt::fmt)
if(MULTIPROCESSING)
    target_compile_definitions(x64emulator PUBLIC MULTIPROCESSING=1)
    target_link_libraries(emulator PUBLIC pthread)
endif(MULTIPROCESSING)

# target_compile_options(emulator PUBLIC -fsanitize=address,undefined)
# target_link_libraries(emulator PUBLIC asan ubsan)