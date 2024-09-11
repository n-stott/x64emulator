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
    src/x64/checkedcpuimpl.cpp
    src/x64/cpuimpl.cpp
    src/x64/simd.cpp
    src/x64/x87.cpp
    src/kernel/host.cpp
    src/utils.cpp
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
    src/emulator/emulator.cpp
    src/emulator/profilingdata.cpp
    src/emulator/symbolprovider.cpp
    src/emulator/vm.cpp
    src/kernel/auxiliaryvector.cpp
    src/kernel/fs/epoll.cpp
    src/kernel/fs/event.cpp
    src/kernel/fs/fs.cpp
    src/kernel/fs/hostfile.cpp
    src/kernel/fs/shadowfile.cpp
    src/kernel/fs/socket.cpp
    src/kernel/fs/stream.cpp
    src/kernel/kernel.cpp
    src/kernel/scheduler.cpp
    src/kernel/syscalls.cpp
    src/kernel/thread.cpp
    src/x64/disassembler/capstonewrapper.cpp
    src/x64/instructions/x64instruction.cpp
    src/x64/cpu.cpp
    src/x64/cpuimpl.cpp
    src/x64/checkedcpuimpl.cpp
    src/x64/mmu.cpp
    src/x64/registers.cpp
    src/x64/simd.cpp
    src/x64/x87.cpp
    src/kernel/host.cpp
    src/utils.cpp
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
target_link_libraries(x64emulator PUBLIC
    ${CAPSTONE_LIBRARIES}
    fmt::fmt
    nlohmann_json::nlohmann_json
)
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