cmake_minimum_required(VERSION 3.13)
project(x64)
set(CMAKE_CXX_STANDARD 17)

set(CC_OPTIONS -g -O0 -DDEBUG -fpic)
set(LD_OPTIONS)

add_executable(test_dead_write src/test_dead_write.cpp)
target_compile_options(test_dead_write PUBLIC ${CC_OPTIONS})
target_include_directories(test_dead_write PUBLIC include ${CMAKE_SOURCE_DIR}/emulator/include)
target_link_libraries(test_dead_write PUBLIC x64cpu x64emulator)
target_link_options(test_dead_write PUBLIC ${LD_OPTIONS})
add_test(NAME dead_write COMMAND test_dead_write)