cmake_minimum_required(VERSION 3.13)
project(x86)
set(CMAKE_CXX_STANDARD 17)

set(CC_OPTIONS -m32 -g -O0 -DDEBUG -fpic -Wall -Wextra -Wpedantic -pedantic -Werror -Wno-sign-compare -Wno-braced-scalar-init -Wimplicit-fallthrough)
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


add_executable(translator 
    src/translation/x86instruction.cpp
    src/parser/parser.cpp
    src/main.cpp
)
target_compile_options(translator PUBLIC -fpic -Wall -Wextra -Wpedantic -pedantic -Werror -Wno-sign-compare -Wno-braced-scalar-init -Wimplicit-fallthrough -Wno-missing-braces)
target_include_directories(translator PUBLIC include)
target_include_directories(translator PUBLIC include ../common/include)
target_include_directories(translator PUBLIC ../external)
target_link_libraries(translator ${CMAKE_SOURCE_DIR}/external/fmt/libfmt.a)