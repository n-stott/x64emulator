cmake_minimum_required(VERSION 3.13)
project(x86)
set(CMAKE_CXX_STANDARD 17)

set(CC_OPTIONS -g -O0 -DDEBUG -fpic -Wall -Wextra -Wpedantic -pedantic -Werror -Wno-sign-compare -Wno-braced-scalar-init -Wimplicit-fallthrough)

add_executable(trivial tests/src/trivial.cpp)
target_compile_options(trivial PUBLIC ${CC_OPTIONS})

add_executable(add tests/src/add.cpp)
target_compile_options(add PUBLIC ${CC_OPTIONS})

add_executable(call0 tests/src/call0.cpp)
target_compile_options(call0 PUBLIC ${CC_OPTIONS})

add_executable(call1 tests/src/call1.cpp)
target_compile_options(call1 PUBLIC ${CC_OPTIONS})