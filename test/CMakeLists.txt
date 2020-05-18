project(project_test)

set (CMAKE_CXX_STANDARD 11)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__VTableFILE__='\"$(subst ${CMAKE_SOURCE_DIR}/,,$(abspath $<))\"'")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__VTableFILE__='\"$(subst ${CMAKE_SOURCE_DIR}/,,$(abspath $<))\"'")


############ Files  ##############################################################################
set(LOCAL_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/src/include)
set(OPENSSL_INCLUDE_DIRS /usr/local/Cellar/openssl/1.0.2s/include)
set(OPENSSL_LIB_DIRS /usr/local/Cellar/openssl/1.0.2s/lib)
#message(${CMAKE_SOURCE_DIR}/src/log/logger.c)

# c get files
file(GLOB test_srcs_temp ${PROJECT_SOURCE_DIR}/*/test_*.cpp)
file(GLOB openssl_libsrcs ${OPENSSL_LIB_DIRS}/*.dylib)
set(test_srcs "")
foreach(test_src_temp ${test_srcs_temp} )
    string(REPLACE "//" "/" test_src ${test_src_temp})
    list(APPEND test_srcs ${test_src})
endforeach(test_src_temp ${test_srcs_temp})

# gmock
set(GMOCK_DIR  "${CMAKE_SOURCE_DIR}/third_party/gmock")
file(GLOB gmock_srcs  ${GMOCK_DIR}/*.cc)
include_directories(SYSTEM ${GMOCK_DIR})
add_library(gtest EXCLUDE_FROM_ALL ${gmock_srcs})
target_link_libraries(gtest ${CMAKE_THREAD_LIBS_INIT})

# memcheck
find_program(MEMORYCHECK_COMMAND valgrind REQUIRED)
# Note you can add '--gen-suppressions=all' to MEMORYCHECK_COMMAND_OPTIONS
# if you want valgrind to print out the syntax to use to suppress a particular
# memory leak
set(MEMORYCHECK_COMMAND_OPTIONS
        --trace-children=yes
        --leak-check=full
        --track-origins=yes
        --soname-synonyms=somalloc=*jemalloc*
        --error-exitcode=1)
set(MEMORYCHECK_SUPPRESSIONS_FILE ${CMAKE_SOURCE_DIR}/third_party/valgrind/valgrind.supp)


############# Executables #############################################################################

foreach(test_src ${test_srcs} )
    # get test file name
    get_filename_component(test_bare_name ${test_src} NAME)
    string(REPLACE ".cpp" "" test_bare_name_without_extension ${test_bare_name})
    string(REPLACE "\"" "" test_name ${test_bare_name_without_extension})
#    message(${CMAKE_SOURCE_DIR}/src/log/logger.c)
    # create executable
    add_executable(${test_name}
            ${test_src}
            ${CMAKE_SOURCE_DIR}/src/log/log.cpp

#            ${CMAKE_SOURCE_DIR}/src/parser/parse.c
#            ${CMAKE_SOURCE_DIR}/src/parser/
#            ${CMAKE_SOURCE_DIR}/src/client_pool/client_pool.cpp

            )

    # include
    target_include_directories(${test_name} PUBLIC
            ${LOCAL_INCLUDE_DIRS}
            ${OPENSSL_INCLUDE_DIRS}
            )

    # link libraries
    target_link_libraries(${test_name}
            ${openssl_libsrcs}
            gtest
            )

    # set target properties
    set_target_properties(${test_name} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test"
            COMMAND ${test_name}
            )

    # add test
    add_test(${test_name} ${MEMORYCHECK_COMMAND} ${MEMORYCHECK_COMMAND_OPTIONS}
            --suppressions=${MEMORYCHECK_SUPPRESSIONS_FILE} ${CMAKE_BINARY_DIR}/test/${test_name}
            --gtest_color=yes --gtest_output=xml:${CMAKE_BINARY_DIR}/test/unit_${test_name}.xml)
    add_test(${test_name} ${CMAKE_BINARY_DIR}/test/${test_name} --gtest_color=yes
            --gtest_output=xml:${CMAKE_BINARY_DIR}/test/${test_name}.xml)

endforeach(test_src ${test_srcs})