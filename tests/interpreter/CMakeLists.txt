cmake_minimum_required(VERSION 3.13)
project(x86)
set(CMAKE_CXX_STANDARD 17)

set(CC_OPTIONS -m32 -g -O0 -DDEBUG -march=i386)
set(LD_OPTIONS -m32)

add_executable(trivial src/trivial.cpp)
target_compile_options(trivial PUBLIC ${CC_OPTIONS})
target_link_options(trivial PUBLIC ${LD_OPTIONS})

add_executable(add src/add.cpp)
target_compile_options(add PUBLIC ${CC_OPTIONS})
target_link_options(add PUBLIC ${LD_OPTIONS})

add_executable(mul src/mul.cpp)
target_compile_options(mul PUBLIC ${CC_OPTIONS})
target_link_options(mul PUBLIC ${LD_OPTIONS})

add_executable(call0 src/call0.cpp)
target_compile_options(call0 PUBLIC ${CC_OPTIONS})
target_link_options(call0 PUBLIC ${LD_OPTIONS})

add_executable(call1 src/call1.cpp)
target_compile_options(call1 PUBLIC ${CC_OPTIONS})
target_link_options(call1 PUBLIC ${LD_OPTIONS})

add_executable(print src/print.cpp)
target_compile_options(print PUBLIC ${CC_OPTIONS})
target_link_options(print PUBLIC ${LD_OPTIONS})

add_executable(arrays src/arrays.cpp)
target_compile_options(arrays PUBLIC ${CC_OPTIONS})
target_link_options(arrays PUBLIC ${LD_OPTIONS})

add_executable(strings src/strings.cpp)
target_compile_options(strings PUBLIC ${CC_OPTIONS})
target_link_options(strings PUBLIC ${LD_OPTIONS} -static-libstdc++ -static-libgcc)

add_executable(malloc src/malloc.cpp)
target_compile_options(malloc PUBLIC ${CC_OPTIONS})
target_link_options(malloc PUBLIC ${LD_OPTIONS})

add_executable(fmt_print src/fmt_print.cpp)
target_compile_options(fmt_print PUBLIC ${CC_OPTIONS} -DFMT_HEADER_ONLY)
target_include_directories(fmt_print PUBLIC ../external)
target_link_options(fmt_print PUBLIC ${LD_OPTIONS} -static-libstdc++ -static-libgcc)

add_executable(tostring src/tostring.cpp)
target_compile_options(tostring PUBLIC ${CC_OPTIONS})
target_link_options(tostring PUBLIC ${LD_OPTIONS} -static-libstdc++ -static-libgcc)

add_executable(comparisons src/comparisons.cpp)
target_compile_options(comparisons PUBLIC ${CC_OPTIONS})
target_link_options(comparisons PUBLIC ${LD_OPTIONS})

add_executable(staticinitialization src/staticinitialization.cpp)
target_compile_options(staticinitialization PUBLIC ${CC_OPTIONS})
target_link_options(staticinitialization PUBLIC ${LD_OPTIONS} -static-libstdc++ -static-libgcc)

add_executable(statics src/statics.cpp)
target_compile_options(statics PUBLIC ${CC_OPTIONS})
target_link_options(statics PUBLIC ${LD_OPTIONS} -static-libstdc++ -static-libgcc)

add_executable(staticcollection src/staticcollection.cpp)
target_compile_options(staticcollection PUBLIC ${CC_OPTIONS})
target_link_options(staticcollection PUBLIC ${LD_OPTIONS} -static-libstdc++ -static-libgcc)

add_executable(staticconstructor src/staticconstructor.cpp)
target_compile_options(staticconstructor PUBLIC ${CC_OPTIONS})
target_link_options(staticconstructor PUBLIC ${LD_OPTIONS} -static-libstdc++ -static-libgcc)

add_executable(exceptions src/exceptions.cpp)
target_compile_options(exceptions PUBLIC ${CC_OPTIONS})
target_link_options(exceptions PUBLIC ${LD_OPTIONS} -static-libstdc++ -static-libgcc)

add_executable(boostmap src/boostmap.cpp)
target_compile_options(boostmap PUBLIC ${CC_OPTIONS})
target_link_options(boostmap PUBLIC ${LD_OPTIONS} -static-libstdc++ -static-libgcc)

add_executable(stringstream src/stringstream.cpp)
target_compile_options(stringstream PUBLIC ${CC_OPTIONS})
target_link_options(stringstream PUBLIC ${LD_OPTIONS} -static-libstdc++ -static-libgcc)

add_executable(interval src/interval.cpp)
target_compile_options(interval PUBLIC ${CC_OPTIONS})
target_link_options(interval PUBLIC ${LD_OPTIONS} -static-libstdc++ -static-libgcc)