cmake_minimum_required(VERSION 3.14)
project(x64)
set(CMAKE_CXX_STANDARD 17)


include(FindPkgConfig)
pkg_check_modules (CAPSTONE REQUIRED capstone)


include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(fmt)


add_executable(elf-reader
    src/elf-reader/main.cpp
)
target_compile_options(elf-reader PUBLIC -Wall -Wextra -Wpedantic -pedantic -Wconversion -Werror)
target_compile_options(elf-reader PUBLIC -g)
target_include_directories(elf-reader PUBLIC include)
target_link_libraries(elf-reader PUBLIC fmt::fmt)


add_library(x64 STATIC
    src/interpreter/cpuimpl.cpp
    src/interpreter/simd.cpp
    src/interpreter/x87.cpp
    src/host/host.cpp
    src/utils/utils.cpp
)
target_compile_options(x64 PUBLIC -Wall -Wextra -Wpedantic -pedantic -Wconversion -Werror -Wimplicit-fallthrough)
target_compile_options(x64 PUBLIC -Wno-braced-scalar-init -Wno-missing-braces)
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    target_compile_options(x64 PUBLIC -Wno-maybe-uninitialized -Wno-stringop-overflow)
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    target_compile_options(x64 PUBLIC -Wno-uninitialized)
endif()
target_compile_options(x64 PUBLIC -frounding-math)
target_include_directories(x64 PUBLIC include)


add_library(emulator STATIC
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
target_compile_options(emulator PUBLIC -Wall -Wextra -Wpedantic -pedantic -Wconversion -Werror -Wimplicit-fallthrough)
target_compile_options(emulator PUBLIC -Wno-braced-scalar-init -Wno-missing-braces)
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    target_compile_options(emulator PUBLIC -Wno-maybe-uninitialized -Wno-stringop-overflow)
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    target_compile_options(emulator PUBLIC -Wno-uninitialized)
endif()
target_compile_options(emulator PUBLIC -frounding-math)
target_compile_options(emulator PUBLIC -DFMT_HEADER_ONLY)
target_include_directories(emulator PUBLIC include)
target_link_libraries(emulator PUBLIC ${CAPSTONE_LIBRARIES} fmt::fmt)

# target_compile_options(emulator PUBLIC -fsanitize=address,undefined)
# target_link_libraries(emulator PUBLIC asan ubsan)

add_executable(interpreter
    src/main.cpp
)
target_compile_options(interpreter PUBLIC -Wall -Wextra -Wpedantic -pedantic -Wconversion -Werror -Wimplicit-fallthrough)
target_compile_options(interpreter PUBLIC -Wno-braced-scalar-init -Wno-missing-braces)
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    target_compile_options(interpreter PUBLIC -Wno-maybe-uninitialized -Wno-stringop-overflow)
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    target_compile_options(interpreter PUBLIC -Wno-uninitialized)
endif()
target_include_directories(interpreter PUBLIC include)
target_include_directories(interpreter PUBLIC ../common/include)
target_link_libraries(interpreter PUBLIC emulator fmt::fmt pthread)

# target_compile_options(interpreter PUBLIC -fsanitize=address,undefined)
# target_link_libraries(interpreter PUBLIC asan ubsan)

enable_testing()
add_subdirectory(tests)