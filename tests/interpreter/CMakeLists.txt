cmake_minimum_required(VERSION 3.13)
project(x64)
set(CMAKE_CXX_STANDARD 17)

set(CC_OPTIONS -g -O0 -DDEBUG)
set(LD_OPTIONS)

add_executable(test_trivial src/test_trivial.cpp)
target_compile_options(test_trivial PUBLIC ${CC_OPTIONS})
target_link_options(test_trivial PUBLIC ${LD_OPTIONS})
add_test(NAME trivial COMMAND ${CMAKE_BINARY_DIR}/interpreter test_trivial)

add_executable(test_add src/test_add.cpp)
target_compile_options(test_add PUBLIC ${CC_OPTIONS})
target_link_options(test_add PUBLIC ${LD_OPTIONS})
add_test(NAME add COMMAND ${CMAKE_BINARY_DIR}/interpreter test_add)

add_executable(test_mul src/test_mul.cpp)
target_compile_options(test_mul PUBLIC ${CC_OPTIONS})
target_link_options(test_mul PUBLIC ${LD_OPTIONS})
add_test(NAME mul COMMAND ${CMAKE_BINARY_DIR}/interpreter test_mul)

add_executable(test_call0 src/test_call0.cpp)
target_compile_options(test_call0 PUBLIC ${CC_OPTIONS})
target_link_options(test_call0 PUBLIC ${LD_OPTIONS})
add_test(NAME call0 COMMAND ${CMAKE_BINARY_DIR}/interpreter test_call0)

add_executable(test_call1 src/test_call1.cpp)
target_compile_options(test_call1 PUBLIC ${CC_OPTIONS})
target_link_options(test_call1 PUBLIC ${LD_OPTIONS})
add_test(NAME call1 COMMAND ${CMAKE_BINARY_DIR}/interpreter test_call1)

add_executable(test_print src/test_print.cpp)
target_compile_options(test_print PUBLIC ${CC_OPTIONS})
target_link_options(test_print PUBLIC ${LD_OPTIONS})
add_test(NAME print COMMAND ${CMAKE_BINARY_DIR}/interpreter test_print)

add_executable(test_arrays src/test_arrays.cpp)
target_compile_options(test_arrays PUBLIC ${CC_OPTIONS})
target_link_options(test_arrays PUBLIC ${LD_OPTIONS})
add_test(NAME arrays COMMAND ${CMAKE_BINARY_DIR}/interpreter test_arrays)

add_executable(test_strings src/test_strings.cpp)
target_compile_options(test_strings PUBLIC ${CC_OPTIONS})
target_link_options(test_strings PUBLIC ${LD_OPTIONS} -static-libstdc++ -static-libgcc)
add_test(NAME strings COMMAND ${CMAKE_BINARY_DIR}/interpreter test_strings)

add_executable(test_malloc src/test_malloc.cpp)
target_compile_options(test_malloc PUBLIC ${CC_OPTIONS})
target_link_options(test_malloc PUBLIC ${LD_OPTIONS})
add_test(NAME malloc COMMAND ${CMAKE_BINARY_DIR}/interpreter test_malloc)

add_executable(test_fmt_print src/test_fmt_print.cpp)
target_compile_options(test_fmt_print PUBLIC ${CC_OPTIONS} -DFMT_HEADER_ONLY)
target_include_directories(test_fmt_print PUBLIC ${CMAKE_SOURCE_DIR}/external)
target_link_options(test_fmt_print PUBLIC ${LD_OPTIONS} -static-libstdc++ -static-libgcc)
add_test(NAME fmt_print COMMAND ${CMAKE_BINARY_DIR}/interpreter test_fmt_print)

add_executable(test_tostring src/test_tostring.cpp)
target_compile_options(test_tostring PUBLIC ${CC_OPTIONS})
target_link_options(test_tostring PUBLIC ${LD_OPTIONS} -static-libstdc++ -static-libgcc)
add_test(NAME tostring COMMAND ${CMAKE_BINARY_DIR}/interpreter test_tostring)

add_executable(test_comparisons src/test_comparisons.cpp)
target_compile_options(test_comparisons PUBLIC ${CC_OPTIONS})
target_link_options(test_comparisons PUBLIC ${LD_OPTIONS})
add_test(NAME comparisons COMMAND ${CMAKE_BINARY_DIR}/interpreter test_comparisons)

add_executable(test_staticinitialization src/test_staticinitialization.cpp)
target_compile_options(test_staticinitialization PUBLIC ${CC_OPTIONS})
target_link_options(test_staticinitialization PUBLIC ${LD_OPTIONS} -static-libstdc++ -static-libgcc)
add_test(NAME staticinitialization COMMAND ${CMAKE_BINARY_DIR}/interpreter test_staticinitialization)

add_executable(test_statics src/test_statics.cpp)
target_compile_options(test_statics PUBLIC ${CC_OPTIONS})
target_link_options(test_statics PUBLIC ${LD_OPTIONS} -static-libstdc++ -static-libgcc)
add_test(NAME statics COMMAND ${CMAKE_BINARY_DIR}/interpreter test_statics)

add_executable(test_staticcollection src/test_staticcollection.cpp)
target_compile_options(test_staticcollection PUBLIC ${CC_OPTIONS})
target_link_options(test_staticcollection PUBLIC ${LD_OPTIONS} -static-libstdc++ -static-libgcc)
add_test(NAME staticcollection COMMAND ${CMAKE_BINARY_DIR}/interpreter test_staticcollection)

add_executable(test_staticconstructor src/test_staticconstructor.cpp)
target_compile_options(test_staticconstructor PUBLIC ${CC_OPTIONS})
target_link_options(test_staticconstructor PUBLIC ${LD_OPTIONS} -static-libstdc++ -static-libgcc)
add_test(NAME staticconstructor COMMAND ${CMAKE_BINARY_DIR}/interpreter test_staticconstructor)

add_executable(test_exceptions src/test_exceptions.cpp)
target_compile_options(test_exceptions PUBLIC ${CC_OPTIONS})
target_link_options(test_exceptions PUBLIC ${LD_OPTIONS} -static-libstdc++ -static-libgcc)
add_test(NAME exceptions COMMAND ${CMAKE_BINARY_DIR}/interpreter test_exceptions)

add_executable(test_boostmap src/test_boostmap.cpp)
target_compile_options(test_boostmap PUBLIC ${CC_OPTIONS})
target_link_options(test_boostmap PUBLIC ${LD_OPTIONS} -static-libstdc++ -static-libgcc)
add_test(NAME boostmap COMMAND ${CMAKE_BINARY_DIR}/interpreter test_boostmap)

add_executable(test_stringstream src/test_stringstream.cpp)
target_compile_options(test_stringstream PUBLIC ${CC_OPTIONS})
target_link_options(test_stringstream PUBLIC ${LD_OPTIONS} -static-libstdc++ -static-libgcc)
add_test(NAME stringstream COMMAND ${CMAKE_BINARY_DIR}/interpreter test_stringstream)

add_executable(test_interval src/test_interval.cpp)
target_compile_options(test_interval PUBLIC ${CC_OPTIONS})
target_link_options(test_interval PUBLIC ${LD_OPTIONS} -static-libstdc++ -static-libgcc)
add_test(NAME interval COMMAND ${CMAKE_BINARY_DIR}/interpreter test_interval)

add_executable(test_argc src/test_argc.cpp)
target_compile_options(test_argc PUBLIC ${CC_OPTIONS})
target_link_options(test_argc PUBLIC ${LD_OPTIONS})
add_test(NAME argc COMMAND ${CMAKE_BINARY_DIR}/interpreter test_argc)

add_executable(test_programargs src/test_programargs.cpp)
target_compile_options(test_programargs PUBLIC ${CC_OPTIONS} -DFMT_HEADER_ONLY)
target_include_directories(test_programargs PUBLIC ${CMAKE_SOURCE_DIR}/external)
target_link_options(test_programargs PUBLIC ${LD_OPTIONS} -static-libstdc++ -static-libgcc)
add_test(NAME programargs COMMAND ${CMAKE_BINARY_DIR}/interpreter test_programargs)

add_executable(test_nontrivialstatic src/test_nontrivialstatic.cpp)
target_compile_options(test_nontrivialstatic PUBLIC ${CC_OPTIONS})
target_link_options(test_nontrivialstatic PUBLIC ${LD_OPTIONS} -static-libstdc++ -static-libgcc)
add_test(NAME nontrivialstatic COMMAND ${CMAKE_BINARY_DIR}/interpreter test_nontrivialstatic)

add_executable(test_threadlocal src/test_threadlocal.cpp)
target_compile_options(test_threadlocal PUBLIC ${CC_OPTIONS})
target_link_options(test_threadlocal PUBLIC ${LD_OPTIONS} -static-libstdc++ -static-libgcc)
add_test(NAME threadlocal COMMAND ${CMAKE_BINARY_DIR}/interpreter test_threadlocal)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/testfile.txt
    ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)

add_executable(test_openfile src/test_openfile.cpp)
target_compile_options(test_openfile PUBLIC ${CC_OPTIONS})
target_link_options(test_openfile PUBLIC ${LD_OPTIONS} -static-libstdc++ -static-libgcc)
add_test(NAME openfile COMMAND ${CMAKE_BINARY_DIR}/interpreter test_openfile)

add_executable(test_readfile src/test_readfile.cpp)
target_compile_options(test_readfile PUBLIC ${CC_OPTIONS})
target_link_options(test_readfile PUBLIC ${LD_OPTIONS} -static-libstdc++ -static-libgcc)
add_test(NAME readfile COMMAND ${CMAKE_BINARY_DIR}/interpreter test_readfile)

add_executable(test_virtual src/test_virtual.cpp)
target_compile_options(test_virtual PUBLIC ${CC_OPTIONS})
target_link_options(test_virtual PUBLIC ${LD_OPTIONS} -static-libstdc++ -static-libgcc)
add_test(NAME virtual COMMAND ${CMAKE_BINARY_DIR}/interpreter test_virtual)

add_executable(test_fpu src/test_fpu.cpp)
target_compile_options(test_fpu PUBLIC ${CC_OPTIONS})
target_link_options(test_fpu PUBLIC ${LD_OPTIONS})
add_test(NAME fpu COMMAND ${CMAKE_BINARY_DIR}/interpreter test_fpu)