cmake_minimum_required(VERSION 3.13)
project(x86)
set(CMAKE_CXX_STANDARD 17)

set(CC_OPTIONS -m32 -g -O0 -DDEBUG -march=i386)
set(LD_OPTIONS -m32)

add_executable(softfloat_add src/softfloat_add.cpp)
target_compile_options(softfloat_add PUBLIC ${CC_OPTIONS})
target_link_options(softfloat_add PUBLIC ${LD_OPTIONS})
target_include_directories(softfloat_add PUBLIC ../../external/berkeley-softfloat-3/source/include)
target_link_libraries(softfloat_add ${CMAKE_SOURCE_DIR}/x86/external/berkeley-softfloat-3/build/Linux-x86_64-GCC/softfloat.a)