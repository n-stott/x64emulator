cmake_minimum_required(VERSION 3.13)
project(x86)
set(CMAKE_CXX_STANDARD 17)

set(CC_OPTIONS -m32 -g -O0 -DDEBUG -march=i386)
set(LD_OPTIONS -m32)

add_executable(trivial tests/src/trivial.cpp)
target_compile_options(trivial PUBLIC ${CC_OPTIONS})
target_link_options(trivial PUBLIC ${LD_OPTIONS})

add_executable(add tests/src/add.cpp)
target_compile_options(add PUBLIC ${CC_OPTIONS})
target_link_options(add PUBLIC ${LD_OPTIONS})

add_executable(call0 tests/src/call0.cpp)
target_compile_options(call0 PUBLIC ${CC_OPTIONS})
target_link_options(call0 PUBLIC ${LD_OPTIONS})

add_executable(call1 tests/src/call1.cpp)
target_compile_options(call1 PUBLIC ${CC_OPTIONS})
target_link_options(call1 PUBLIC ${LD_OPTIONS})

add_executable(print tests/src/print.cpp)
target_compile_options(print PUBLIC ${CC_OPTIONS})
target_link_options(print PUBLIC ${LD_OPTIONS})

add_executable(arrays tests/src/arrays.cpp)
target_compile_options(arrays PUBLIC ${CC_OPTIONS})
target_link_options(arrays PUBLIC ${LD_OPTIONS})

add_executable(strings tests/src/strings.cpp)
target_compile_options(strings PUBLIC ${CC_OPTIONS})
target_link_options(strings PUBLIC ${LD_OPTIONS} -static-libstdc++ -static-libgcc)

add_library(fakelibc SHARED
    src/lib/fakelibc.cpp
)
target_compile_options(fakelibc PUBLIC -m32 -fPIC -O2)
target_link_options(fakelibc PUBLIC -m32)

add_executable(translator
    external/elf-reader/src/elf-reader.cpp
    src/lib/library.cpp
    src/lib/libc.cpp
    src/translation/x86instruction.cpp
    src/parser/disassembler.cpp
    src/parser/parser.cpp
    src/interpreter/interpreter.cpp
    src/interpreter/mmu.cpp
    src/interpreter/executioncontext.cpp
    src/program.cpp
    src/main.cpp
)
target_compile_options(translator PUBLIC -fpic -Wall -Wextra -Wpedantic -pedantic -Werror -Wno-sign-compare -Wno-braced-scalar-init -Wimplicit-fallthrough -Wno-missing-braces)
target_include_directories(translator PUBLIC include)
target_include_directories(translator PUBLIC ../common/include)
target_include_directories(translator PUBLIC ../external)
target_include_directories(translator PUBLIC external/elf-reader/src)
target_link_libraries(translator ${CMAKE_SOURCE_DIR}/external/fmt/libfmt.a)