cmake_minimum_required(VERSION 3.13)
project(x86)
set(CMAKE_CXX_STANDARD 17)

set(CC_OPTIONS -m32 -g -O0 -DDEBUG -march=i386)
set(LD_OPTIONS -m32)

add_executable(test_trivial src/test_trivial.cpp)
target_compile_options(test_trivial PUBLIC ${CC_OPTIONS})
target_link_options(test_trivial PUBLIC ${LD_OPTIONS})

add_executable(test_add src/test_add.cpp)
target_compile_options(test_add PUBLIC ${CC_OPTIONS})
target_link_options(test_add PUBLIC ${LD_OPTIONS})

add_executable(test_mul src/test_mul.cpp)
target_compile_options(test_mul PUBLIC ${CC_OPTIONS})
target_link_options(test_mul PUBLIC ${LD_OPTIONS})

add_executable(test_call0 src/test_call0.cpp)
target_compile_options(test_call0 PUBLIC ${CC_OPTIONS})
target_link_options(test_call0 PUBLIC ${LD_OPTIONS})

add_executable(test_call1 src/test_call1.cpp)
target_compile_options(test_call1 PUBLIC ${CC_OPTIONS})
target_link_options(test_call1 PUBLIC ${LD_OPTIONS})

add_executable(test_print src/test_print.cpp)
target_compile_options(test_print PUBLIC ${CC_OPTIONS})
target_link_options(test_print PUBLIC ${LD_OPTIONS})

add_executable(test_arrays src/test_arrays.cpp)
target_compile_options(test_arrays PUBLIC ${CC_OPTIONS})
target_link_options(test_arrays PUBLIC ${LD_OPTIONS})

add_executable(test_strings src/test_strings.cpp)
target_compile_options(test_strings PUBLIC ${CC_OPTIONS})
target_link_options(test_strings PUBLIC ${LD_OPTIONS} -static-libstdc++ -static-libgcc)

add_executable(test_malloc src/test_malloc.cpp)
target_compile_options(test_malloc PUBLIC ${CC_OPTIONS})
target_link_options(test_malloc PUBLIC ${LD_OPTIONS})

add_executable(test_fmt_print src/test_fmt_print.cpp)
target_compile_options(test_fmt_print PUBLIC ${CC_OPTIONS} -DFMT_HEADER_ONLY)
target_include_directories(test_fmt_print PUBLIC ../external)
target_link_options(test_fmt_print PUBLIC ${LD_OPTIONS} -static-libstdc++ -static-libgcc)

add_executable(test_tostring src/test_tostring.cpp)
target_compile_options(test_tostring PUBLIC ${CC_OPTIONS})
target_link_options(test_tostring PUBLIC ${LD_OPTIONS} -static-libstdc++ -static-libgcc)

add_executable(test_comparisons src/test_comparisons.cpp)
target_compile_options(test_comparisons PUBLIC ${CC_OPTIONS})
target_link_options(test_comparisons PUBLIC ${LD_OPTIONS})

add_executable(test_staticinitialization src/test_staticinitialization.cpp)
target_compile_options(test_staticinitialization PUBLIC ${CC_OPTIONS})
target_link_options(test_staticinitialization PUBLIC ${LD_OPTIONS} -static-libstdc++ -static-libgcc)

add_executable(test_statics src/test_statics.cpp)
target_compile_options(test_statics PUBLIC ${CC_OPTIONS})
target_link_options(test_statics PUBLIC ${LD_OPTIONS} -static-libstdc++ -static-libgcc)

add_executable(test_staticcollection src/test_staticcollection.cpp)
target_compile_options(test_staticcollection PUBLIC ${CC_OPTIONS})
target_link_options(test_staticcollection PUBLIC ${LD_OPTIONS} -static-libstdc++ -static-libgcc)

add_executable(test_staticconstructor src/test_staticconstructor.cpp)
target_compile_options(test_staticconstructor PUBLIC ${CC_OPTIONS})
target_link_options(test_staticconstructor PUBLIC ${LD_OPTIONS} -static-libstdc++ -static-libgcc)

add_executable(test_exceptions src/test_exceptions.cpp)
target_compile_options(test_exceptions PUBLIC ${CC_OPTIONS})
target_link_options(test_exceptions PUBLIC ${LD_OPTIONS} -static-libstdc++ -static-libgcc)

add_executable(test_boostmap src/test_boostmap.cpp)
target_compile_options(test_boostmap PUBLIC ${CC_OPTIONS})
target_link_options(test_boostmap PUBLIC ${LD_OPTIONS} -static-libstdc++ -static-libgcc)

add_executable(test_stringstream src/test_stringstream.cpp)
target_compile_options(test_stringstream PUBLIC ${CC_OPTIONS})
target_link_options(test_stringstream PUBLIC ${LD_OPTIONS} -static-libstdc++ -static-libgcc)

add_executable(test_interval src/test_interval.cpp)
target_compile_options(test_interval PUBLIC ${CC_OPTIONS})
target_link_options(test_interval PUBLIC ${LD_OPTIONS} -static-libstdc++ -static-libgcc)

add_executable(test_programargs src/test_programargs.cpp)
target_compile_options(test_programargs PUBLIC ${CC_OPTIONS} -DFMT_HEADER_ONLY)
target_include_directories(test_programargs PUBLIC ../external)
target_link_options(test_programargs PUBLIC ${LD_OPTIONS} -static-libstdc++ -static-libgcc)