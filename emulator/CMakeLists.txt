cmake_minimum_required(VERSION 3.14)
project(x64)
set(CMAKE_CXX_STANDARD 17)

add_executable(elf-reader
    src/elf-reader/main.cpp
)
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    target_compile_options(elf-reader PRIVATE -Wall -Wextra -Wpedantic -pedantic -Wconversion -Werror)
    target_compile_options(elf-reader PRIVATE -Wno-maybe-uninitialized -Wno-stringop-overflow)
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    target_compile_options(elf-reader PRIVATE -Wall -Wextra -Wpedantic -pedantic -Wconversion -Werror)
    target_compile_options(elf-reader PRIVATE -Wno-uninitialized)
endif()
target_include_directories(elf-reader PRIVATE include)
target_link_libraries(elf-reader PRIVATE fmt::fmt-header-only)

add_executable(pe-reader
    src/pe-reader/main.cpp
)
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    target_compile_options(pe-reader PRIVATE -Wall -Wextra -Wpedantic -pedantic -Wconversion -Werror)
    target_compile_options(pe-reader PRIVATE -Wno-maybe-uninitialized -Wno-stringop-overflow)
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    target_compile_options(pe-reader PRIVATE -Wall -Wextra -Wpedantic -pedantic -Wconversion -Werror)
    target_compile_options(pe-reader PRIVATE -Wno-uninitialized)
endif()
target_include_directories(pe-reader PRIVATE include)
target_link_libraries(pe-reader PRIVATE fmt::fmt-header-only)
target_compile_definitions(pe-reader PRIVATE ${COMPILE_DEFINITIONS})

if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    set(COMPILE_OPTIONS
        -Wall -Wextra -Wpedantic -pedantic -Wconversion -Werror -Wimplicit-fallthrough
        -Wno-braced-scalar-init -Wno-missing-braces -Wno-maybe-uninitialized -Wno-stringop-overflow
        -frounding-math -fno-omit-frame-pointer
        -DFMT_HEADER_ONLY
    )
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    set(COMPILE_OPTIONS
        -Wall -Wextra -Wpedantic -pedantic -Wconversion -Werror -Wimplicit-fallthrough
        -Wno-braced-scalar-init -Wno-missing-braces -Wno-uninitialized -Wno-nested-anon-types
        -frounding-math -fno-omit-frame-pointer
        -DFMT_HEADER_ONLY
    )
endif()

set(COMPILE_DEFINITIONS "")
if(MULTIPROCESSING)
    set(COMPILE_DEFINITIONS ${COMPILE_DEFINITIONS} MULTIPROCESSING=1)
endif(MULTIPROCESSING)

if(SSE3)
    set(COMPILE_OPTIONS ${COMPILE_OPTIONS} -msse3)
    set(COMPILE_DEFINITIONS ${COMPILE_DEFINITIONS} SSE3)
endif(SSE3)

if(SSSE3)
    set(COMPILE_OPTIONS ${COMPILE_OPTIONS} -mssse3)
    set(COMPILE_DEFINITIONS ${COMPILE_DEFINITIONS} SSSE3)
endif(SSSE3)

if(SSE41)
    set(COMPILE_OPTIONS ${COMPILE_OPTIONS} -msse4.1)
    set(COMPILE_DEFINITIONS ${COMPILE_DEFINITIONS} SSE41)
endif(SSE41)

if(SSE42)
    set(COMPILE_OPTIONS ${COMPILE_OPTIONS} -msse4.2)
    set(COMPILE_DEFINITIONS ${COMPILE_DEFINITIONS} SSE42)
endif(SSE42)

set(LIB_KIND STATIC)

add_library(x64cpu ${LIB_KIND}
    src/host/hostinstructions.cpp
    src/host/hostmemory.cpp
    src/mem/mmu.cpp
    src/arch/x64/checkedcpuimpl.cpp
    src/arch/x64/cpu.cpp
    src/arch/x64/cpuimpl.cpp
    src/arch/x64/flags.cpp
    src/arch/x64/nativecpuimpl.cpp
    src/arch/x64/registers.cpp
    src/arch/x64/simd.cpp
    src/arch/x64/instructions/instruction.cpp
    src/arch/x64/x87.cpp
    src/arch/x64/types.cpp
)
target_compile_options(x64cpu PUBLIC ${COMPILE_OPTIONS})
target_include_directories(x64cpu PUBLIC
    include
)
if(${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
    target_include_directories(x64cpu PUBLIC
        ${boost_int128_SOURCE_DIR}/include
    )
endif()
target_link_libraries(x64cpu PUBLIC fmt::fmt-header-only)
target_compile_definitions(x64cpu PRIVATE ${COMPILE_DEFINITIONS})

# set_target_properties(x64cpu PROPERTIES CXX_CLANG_TIDY
#     clang-tidy-18;
#     --use-color;
#     -header-filter=${CMAKE_CURRENT_SOURCE_DIR}/include
# )

add_library(arm64cpu ${LIB_KIND}
    src/host/hostmemory.cpp
    src/mem/mmu.cpp
    src/arch/arm64/cpu.cpp
    src/arch/arm64/flags.cpp
    src/arch/arm64/registers.cpp
    src/arch/arm64/instructions/instruction.cpp
)
target_compile_options(arm64cpu PUBLIC ${COMPILE_OPTIONS})
target_include_directories(arm64cpu PUBLIC
    include
)
if(${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
    target_include_directories(arm64cpu PUBLIC
        ${boost_int128_SOURCE_DIR}/include
    )
endif()
target_link_libraries(arm64cpu PUBLIC fmt::fmt-header-only)
target_compile_definitions(arm64cpu PRIVATE ${COMPILE_DEFINITIONS})

add_library(x64jit ${LIB_KIND}
    src/arch/x64/compiler/assembler.cpp
    src/arch/x64/compiler/codegenerator.cpp
    src/arch/x64/compiler/compiler.cpp
    src/arch/x64/compiler/executablememoryallocator.cpp
    src/arch/x64/compiler/ir.cpp
    src/arch/x64/compiler/irgenerator.cpp
    src/arch/x64/compiler/jit.cpp
    src/arch/x64/compiler/optimizer.cpp
    src/arch/x64/disassembler/zydiswrapper.cpp
    src/arch/x64/codesegment.cpp
)
target_compile_options(x64jit PRIVATE ${COMPILE_OPTIONS})
target_include_directories(x64jit PUBLIC
    include
    ${zydis_SOURCE_DIR}/include
    ${zydis_SOURCE_DIR}/dependencies/zycore/include
)
target_link_directories(x64jit PUBLIC
    ${zydis_BINARY_DIR}/
    ${zydis_BINARY_DIR}/zycore/
)
target_link_libraries(x64jit PUBLIC
    x64cpu
    fmt::fmt-header-only
    Zydis
    Zycore
)
target_compile_definitions(x64jit PRIVATE ${COMPILE_DEFINITIONS})


#set_target_properties(x64jit PROPERTIES CXX_CLANG_TIDY
#    clang-tidy-18;
#    --use-color;
#    -header-filter=${CMAKE_CURRENT_SOURCE_DIR}/include
#)


if(NOT ${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
    add_library(linuxkernel ${LIB_KIND}
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
        src/kernel/linux/sys/execve.cpp
        src/kernel/linux/shm/sharedmemory.cpp
        src/kernel/linux/shm/sharedmemorysegment.cpp
        src/kernel/linux/auxiliaryvector.cpp
        src/kernel/linux/kernel.cpp
        src/kernel/linux/process.cpp
        src/kernel/linux/processtable.cpp
        src/kernel/linux/scheduler.cpp
        src/kernel/linux/symbolprovider.cpp
        src/kernel/linux/syscalls.cpp
        src/kernel/linux/thread.cpp
        src/kernel/linux/threadblocker.cpp
        src/kernel/timers.cpp
    )
    target_compile_options(linuxkernel PRIVATE ${COMPILE_OPTIONS})
    target_include_directories(linuxkernel PUBLIC
        include
        ${CMAKE_SOURCE_DIR}/profiling/include
    )
    target_link_directories(linuxkernel PRIVATE ${capstone_BINARY_DIR})
    target_link_libraries(linuxkernel PUBLIC
        profiling
        fmt::fmt-header-only
    )
    target_compile_definitions(linuxkernel PRIVATE ${COMPILE_DEFINITIONS})

    # target_compile_options(linuxkernel PRIVATE -fsanitize=address,undefined -fno-sanitize-recover=all)
    # target_link_libraries(linuxkernel PRIVATE asan ubsan)

    # IF(CMAKE_BUILD_TYPE MATCHES Debug)
    #     set_target_properties(linuxkernel PROPERTIES CXX_CLANG_TIDY
    #         clang-tidy-18;
    #         --use-color;
    #         -header-filter=${CMAKE_CURRENT_SOURCE_DIR}/include
    #     )
    # ENDIF(CMAKE_BUILD_TYPE MATCHES Debug)

    add_executable(emulator
        src/main.cpp
        src/linux-x64-emulator/emulator.cpp
        src/linux-x64-emulator/vm.cpp
        src/linux-x64-emulator/vmprocess.cpp
        src/linux-x64-emulator/vmthread.cpp
    )
    target_compile_options(emulator PRIVATE ${COMPILE_OPTIONS})
    target_include_directories(emulator PRIVATE include)
    target_link_libraries(emulator PUBLIC
        x64cpu
        x64jit
        linuxkernel
        fmt::fmt-header-only
        argparse
    )

    add_executable(arm-emulator
        src/main-arm.cpp
        src/arch/arm64/codesegment.cpp
        src/arch/arm64/disassembler/capstonewrapper.cpp
        src/arch/arm64/instructions/instruction.cpp
        src/linux-arm64-emulator/emulator.cpp
        src/linux-arm64-emulator/vm.cpp
        src/linux-arm64-emulator/vmprocess.cpp
        src/linux-arm64-emulator/vmthread.cpp
    )
    target_compile_options(arm-emulator PRIVATE ${COMPILE_OPTIONS})
    target_include_directories(arm-emulator PRIVATE
        include
        ${capstone_SOURCE_DIR}/include
    )
    target_link_directories(arm-emulator PRIVATE
        ${capstone_BINARY_DIR}/
    )
    target_link_libraries(arm-emulator PUBLIC
        arm64cpu
        linuxkernel
        capstone
        fmt::fmt-header-only
        argparse
    )
endif()

# target_compile_options(emulator PRIVATE -fsanitize=address,undefined -fno-sanitize-recover=all)
# target_link_libraries(emulator PRIVATE asan ubsan)

if(MULTIPROCESSING)
    target_link_libraries(emulator PRIVATE pthread)
endif(MULTIPROCESSING)



add_executable(pe-exec
    src/pe-reader/pe-exec.cpp
)
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    target_compile_options(pe-exec PRIVATE -Wall -Wextra -Wpedantic -pedantic -Wconversion -Werror)
    target_compile_options(pe-exec PRIVATE -Wno-maybe-uninitialized -Wno-stringop-overflow)
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    target_compile_options(pe-exec PRIVATE -Wall -Wextra -Wpedantic -pedantic -Wconversion -Werror)
    target_compile_options(pe-exec PRIVATE -Wno-uninitialized)
endif()
target_include_directories(pe-exec PRIVATE include)
target_link_libraries(pe-exec PRIVATE
    fmt::fmt-header-only
    x64cpu
)
