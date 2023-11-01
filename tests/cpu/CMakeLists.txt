cmake_minimum_required(VERSION 3.13)
project(x64)
set(CMAKE_CXX_STANDARD 17)

set(CC_OPTIONS -g -O0 -DDEBUG -fpic)
set(LD_OPTIONS)

add_executable(test_pshufd src/test_pshufd.cpp)
target_compile_options(test_pshufd PUBLIC ${CC_OPTIONS})
target_compile_options(test_pshufd PUBLIC -DFMT_HEADER_ONLY)
target_include_directories(test_pshufd PUBLIC include ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/external)
target_link_libraries(test_pshufd PUBLIC emulator)
target_link_options(test_pshufd PUBLIC ${LD_OPTIONS})
add_test(NAME pshufd COMMAND test_pshufd)