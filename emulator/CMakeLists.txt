cmake_minimum_required(VERSION 3.14)
project(x64)
set(CMAKE_CXX_STANDARD 17)

add_executable(elf-reader
    src/elf-reader/main.cpp
)
target_compile_options(elf-reader PUBLIC -Wall -Wextra -Wpedantic -pedantic -Wconversion -Werror)
target_compile_options(elf-reader PUBLIC)
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    target_compile_options(elf-reader PUBLIC -Wno-maybe-uninitialized -Wno-stringop-overflow)
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    target_compile_options(elf-reader PUBLIC -Wno-uninitialized)
endif()
target_include_directories(elf-reader PUBLIC include)
target_link_libraries(elf-reader PUBLIC fmt::fmt-header-only)


add_library(x64cpu STATIC
    src/host/hostinstructions.cpp
    src/host/hostmemory.cpp
    src/x64/checkedcpuimpl.cpp
    src/x64/cpu.cpp
    src/x64/cpuimpl.cpp
    src/x64/flags.cpp
    src/x64/mmu.cpp
    src/x64/nativecpuimpl.cpp
    src/x64/registers.cpp
    src/x64/simd.cpp
    src/x64/instructions/x64instruction.cpp
    src/x64/x87.cpp
    src/utils.cpp
)
target_compile_options(x64cpu PUBLIC -Wall -Wextra -Wpedantic -pedantic -Wconversion -Werror -Wimplicit-fallthrough)
target_compile_options(x64cpu PUBLIC -Wno-braced-scalar-init -Wno-missing-braces)
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    target_compile_options(x64cpu PUBLIC -Wno-maybe-uninitialized -Wno-stringop-overflow)
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    target_compile_options(x64cpu PUBLIC -Wno-uninitialized)
endif()
target_compile_options(x64cpu PUBLIC -frounding-math -fno-omit-frame-pointer)
target_include_directories(x64cpu PUBLIC include)
target_link_libraries(x64cpu PUBLIC fmt::fmt-header-only)
target_compile_options(x64cpu PUBLIC
    -DFMT_HEADER_ONLY
    -DFMT_USE_FLOAT=0
    -DFMT_USE_DOUBLE=0
    -DFMT_USE_LONG_DOUBLE=0
)


add_library(x64emulator STATIC
    src/emulator/emulator.cpp
    src/emulator/executablememoryallocator.cpp
    src/emulator/symbolprovider.cpp
    src/emulator/vm.cpp
    src/host/host.cpp
    src/host/hostinstructions.cpp
    src/host/hostmemory.cpp
    src/kernel/auxiliaryvector.cpp
    src/kernel/dev/hostdevice.cpp
    src/kernel/dev/nulldevice.cpp
    src/kernel/dev/shadowdevice.cpp
    src/kernel/dev/tty.cpp
    src/kernel/fs/directory.cpp
    src/kernel/fs/epoll.cpp
    src/kernel/fs/event.cpp
    src/kernel/fs/file.cpp
    src/kernel/fs/fs.cpp
    src/kernel/fs/hostdirectory.cpp
    src/kernel/fs/hostfile.cpp
    src/kernel/fs/localsocket.cpp
    src/kernel/fs/openfiledescription.cpp
    src/kernel/fs/path.cpp
    src/kernel/fs/pipe.cpp
    src/kernel/fs/shadowdirectory.cpp
    src/kernel/fs/shadowfile.cpp
    src/kernel/fs/socket.cpp
    src/kernel/kernel.cpp
    src/kernel/scheduler.cpp
    src/kernel/syscalls.cpp
    src/kernel/thread.cpp
    src/kernel/threadblocker.cpp
    src/kernel/timers.cpp
    src/x64/compiler/assembler.cpp
    src/x64/compiler/codegenerator.cpp
    src/x64/compiler/compiler.cpp
    src/x64/compiler/ir.cpp
    src/x64/compiler/irgenerator.cpp
    src/x64/compiler/optimizer.cpp
    src/x64/disassembler/capstonewrapper.cpp
    src/x64/instructions/x64instruction.cpp
    src/x64/cpu.cpp
    src/x64/cpuimpl.cpp
    src/x64/checkedcpuimpl.cpp
    src/x64/flags.cpp
    src/x64/nativecpuimpl.cpp
    src/x64/mmu.cpp
    src/x64/registers.cpp
    src/x64/simd.cpp
    src/x64/x87.cpp
    src/utils.cpp
)
target_compile_options(x64emulator PUBLIC -Wall -Wextra -Wpedantic -pedantic -Wconversion -Werror -Wimplicit-fallthrough)
target_compile_options(x64emulator PUBLIC -Wno-braced-scalar-init -Wno-missing-braces)
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    target_compile_options(x64emulator PUBLIC -Wno-maybe-uninitialized -Wno-stringop-overflow)
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    target_compile_options(x64emulator PUBLIC -Wno-uninitialized)
endif()
target_compile_options(x64emulator PUBLIC -frounding-math -fno-omit-frame-pointer)
target_compile_options(x64emulator PUBLIC
    -DFMT_HEADER_ONLY
    -DFMT_USE_FLOAT=0
    -DFMT_USE_DOUBLE=0
    -DFMT_USE_LONG_DOUBLE=0
)
target_include_directories(x64emulator PUBLIC include)
target_include_directories(x64emulator PUBLIC ${CMAKE_SOURCE_DIR}/profiling/include)
target_include_directories(x64emulator PUBLIC ${capstone_SOURCE_DIR}/include)
target_link_directories(x64emulator PUBLIC ${capstone_BINARY_DIR})
target_link_libraries(x64emulator PUBLIC
    capstone
    fmt::fmt-header-only
    profiling
)
if(MULTIPROCESSING)
    target_compile_definitions(x64emulator PUBLIC MULTIPROCESSING=1)
endif(MULTIPROCESSING)

# set_property(TARGET x64emulator PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)

# target_compile_options(x64emulator PUBLIC -fsanitize=address,undefined -fno-sanitize-recover=all)
# target_link_libraries(x64emulator PUBLIC asan ubsan)

# IF(CMAKE_BUILD_TYPE MATCHES Debug)
#     set_target_properties(x64emulator PROPERTIES CXX_CLANG_TIDY
#         clang-tidy-18;
#         --use-color;
#         -header-filter=${CMAKE_CURRENT_SOURCE_DIR}/include
#     )
# ENDIF(CMAKE_BUILD_TYPE MATCHES Debug)

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
target_link_libraries(emulator PUBLIC x64emulator fmt::fmt-header-only argparse)

# target_compile_options(emulator PUBLIC -fsanitize=address,undefined -fno-sanitize-recover=all)
# target_link_libraries(emulator PUBLIC asan ubsan)

if(MULTIPROCESSING)
    target_compile_definitions(x64emulator PUBLIC MULTIPROCESSING=1)
    target_link_libraries(emulator PUBLIC pthread)
endif(MULTIPROCESSING)