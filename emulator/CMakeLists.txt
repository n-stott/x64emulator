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

if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    set(COMPILE_OPTIONS
        -Wall -Wextra -Wpedantic -pedantic -Wconversion -Werror -Wimplicit-fallthrough
        -Wno-braced-scalar-init -Wno-missing-braces -Wno-maybe-uninitialized -Wno-stringop-overflow
        -frounding-math -fno-omit-frame-pointer
        -DFMT_HEADER_ONLY -DFMT_USE_FLOAT=0 -DFMT_USE_DOUBLE=0 -DFMT_USE_LONG_DOUBLE=0
    )
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    set(COMPILE_OPTIONS
        -Wall -Wextra -Wpedantic -pedantic -Wconversion -Werror -Wimplicit-fallthrough
        -Wno-braced-scalar-init -Wno-missing-braces -Wno-uninitialized -Wno-nested-anon-types
        -frounding-math -fno-omit-frame-pointer
        -DFMT_HEADER_ONLY -DFMT_USE_FLOAT=0 -DFMT_USE_DOUBLE=0 -DFMT_USE_LONG_DOUBLE=0
    )
endif()

set(LIB_KIND STATIC)

add_library(x64cpu ${LIB_KIND}
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
    src/x64/types.cpp
)
target_compile_options(x64cpu PUBLIC ${COMPILE_OPTIONS})
target_include_directories(x64cpu PUBLIC include)
target_link_libraries(x64cpu PUBLIC fmt::fmt-header-only)
if(MULTIPROCESSING)
    target_compile_definitions(x64cpu PUBLIC MULTIPROCESSING=1)
endif(MULTIPROCESSING)


add_library(x64jit ${LIB_KIND}
    src/x64/compiler/assembler.cpp
    src/x64/compiler/codegenerator.cpp
    src/x64/compiler/compiler.cpp
    src/x64/compiler/executablememoryallocator.cpp
    src/x64/compiler/ir.cpp
    src/x64/compiler/irgenerator.cpp
    src/x64/compiler/jit.cpp
    src/x64/compiler/optimizer.cpp
    src/x64/disassembler/capstonewrapper.cpp
)
target_compile_options(x64jit PUBLIC ${COMPILE_OPTIONS})
target_include_directories(x64jit PUBLIC
    include
    ${capstone_SOURCE_DIR}/include
)
target_link_libraries(x64jit PUBLIC
    x64cpu
    fmt::fmt-header-only
    capstone
)
if(MULTIPROCESSING)
    target_compile_definitions(x64jit PUBLIC MULTIPROCESSING=1)
endif(MULTIPROCESSING)


add_library(x64emulator ${LIB_KIND}
    src/emulator/emulator.cpp
    src/emulator/symbolprovider.cpp
    src/emulator/vm.cpp
    src/emulator/vmthread.cpp
    src/host/host.cpp
    src/kernel/linux/dev/hostdevice.cpp
    src/kernel/linux/dev/nulldevice.cpp
    src/kernel/linux/dev/shadowdevice.cpp
    src/kernel/linux/dev/tty.cpp
    src/kernel/linux/fs/directory.cpp
    src/kernel/linux/fs/epoll.cpp
    src/kernel/linux/fs/event.cpp
    src/kernel/linux/fs/file.cpp
    src/kernel/linux/fs/fs.cpp
    src/kernel/linux/fs/hostdirectory.cpp
    src/kernel/linux/fs/hostfile.cpp
    src/kernel/linux/fs/hostsymlink.cpp
    src/kernel/linux/fs/localsocket.cpp
    src/kernel/linux/fs/openfiledescription.cpp
    src/kernel/linux/fs/path.cpp
    src/kernel/linux/fs/pipe.cpp
    src/kernel/linux/fs/procfs.cpp
    src/kernel/linux/fs/shadowdirectory.cpp
    src/kernel/linux/fs/shadowfile.cpp
    src/kernel/linux/fs/shadowsymlink.cpp
    src/kernel/linux/fs/socket.cpp
    src/kernel/linux/fs/symlink.cpp
    src/kernel/linux/shm/sharedmemory.cpp
    src/kernel/linux/shm/sharedmemorysegment.cpp
    src/kernel/linux/auxiliaryvector.cpp
    src/kernel/linux/kernel.cpp
    src/kernel/linux/syscalls.cpp
    src/kernel/linux/scheduler.cpp
    src/kernel/linux/thread.cpp
    src/kernel/linux/threadblocker.cpp
    src/kernel/timers.cpp
)
target_compile_options(x64emulator PUBLIC ${COMPILE_OPTIONS})
target_include_directories(x64emulator PUBLIC
    include
    ${CMAKE_SOURCE_DIR}/profiling/include
)
target_link_directories(x64emulator PUBLIC ${capstone_BINARY_DIR})
target_link_libraries(x64emulator PUBLIC
    x64cpu
    x64jit
    profiling
    fmt::fmt-header-only
)
if(MULTIPROCESSING)
    target_compile_definitions(x64emulator PUBLIC MULTIPROCESSING=1)
endif(MULTIPROCESSING)

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
target_compile_options(emulator PUBLIC ${COMPILE_OPTIONS})
target_include_directories(emulator PUBLIC include)
target_link_libraries(emulator PUBLIC
    x64emulator
    fmt::fmt-header-only
    argparse
)

# target_compile_options(emulator PUBLIC -fsanitize=address,undefined -fno-sanitize-recover=all)
# target_link_libraries(emulator PUBLIC asan ubsan)

if(MULTIPROCESSING)
    target_compile_definitions(emulator PUBLIC MULTIPROCESSING=1)
    target_link_libraries(emulator PUBLIC pthread)
endif(MULTIPROCESSING)