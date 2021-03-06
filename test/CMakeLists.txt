########## Set up ####################
project(project_test)

set (CMAKE_CXX_STANDARD 11)

# set system
string(REGEX MATCH "Linux"  IS_LINUX "${CMAKE_SYSTEM_NAME}")
string(REGEX MATCH "Darwin" IS_MACOS "${CMAKE_SYSTEM_NAME}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__VTableFILE__='\"$(subst ${CMAKE_SOURCE_DIR}/,,$(abspath $<))\"'")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__VTableFILE__='\"$(subst ${CMAKE_SOURCE_DIR}/,,$(abspath $<))\"'")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__ROOTPATH__='\"${CMAKE_SOURCE_DIR}/root/\"'")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__LOGPATH__='\"${CMAKE_SOURCE_DIR}/log/\"'")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__LOGFILE__='\"${CMAKE_SOURCE_DIR}/log/mylog.log\"'")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__SSLKEY__='\"${CMAKE_SOURCE_DIR}/sslfiles/xiaotons.key\"'")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__SSLCERT__='\"${CMAKE_SOURCE_DIR}/sslfiles/xiaotons.crt\"'")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUG")

add_definitions(-DDEBUG)
#add_definitions(-DDEBUG_VERBOSE)


############ Files  ##############################################################################
set(LOCAL_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/src/include)
if (IS_LINUX)
    set(MYSQL_INCLUDE_DIRS /usr/include)
    set(MYSQL_LIB_DIRS /usr/lib/x86_64-linux-gnu)
    set(OPENSSL_INCLUDE_DIRS /usr/include)
    set(OPENSSL_LIB_DIRS /usr/lib/x86_64-linux-gnu)
    file(GLOB openssl_libsrcs
            ${OPENSSL_LIB_DIRS}/libssl.so
            ${OPENSSL_LIB_DIRS}/libcrypto.so
            )
    file(GLOB_RECURSE mysql_lib_srcs ${MYSQL_LIB_DIRS}/libmysqlclient.so)
elseif(IS_MACOS)
    set(MYSQL_INCLUDE_DIRS /usr/local/Cellar/mysql/8.0.16/include)
    set(MYSQL_LIB_DIRS /usr/local/Cellar/mysql/8.0.16/lib)
    set(OPENSSL_INCLUDE_DIRS /usr/local/Cellar/openssl/1.0.2s/include)
    set(OPENSSL_LIB_DIRS /usr/local/Cellar/openssl/1.0.2s/lib)
    file(GLOB openssl_libsrcs ${OPENSSL_LIB_DIRS}/*.a)
    file(GLOB_RECURSE mysql_lib_srcs ${MYSQL_LIB_DIRS}/*.a)
endif()

#foreach(gg ${openssl_libsrcs})
#    message(${gg})
#endforeach(gg ${openssl_libsrcs})

# c get files
file(GLOB test_srcs_temp ${PROJECT_SOURCE_DIR}/*/test_*.cpp)
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

    # create executable
    add_executable(${test_name}
            ${test_src}
            ${CMAKE_SOURCE_DIR}/src/common/common.cpp
            ${CMAKE_SOURCE_DIR}/src/io/io.cpp
            ${CMAKE_SOURCE_DIR}/src/ssl/ssl.cpp

            ${CMAKE_SOURCE_DIR}/src/log/log.cpp
            ${CMAKE_SOURCE_DIR}/src/CGImysql/sql_connection_pool.cpp
            ${CMAKE_SOURCE_DIR}/src/lock/locker.cpp
            ${CMAKE_SOURCE_DIR}/src/timer/lst_timer.cpp
            ${CMAKE_SOURCE_DIR}/src/signal/connector.cpp

            ${CMAKE_SOURCE_DIR}/src/client_pool/client_pool.cpp
            ${CMAKE_SOURCE_DIR}/src/http/http_conn.cpp

            )

    # include
    target_include_directories(${test_name} PUBLIC
            ${LOCAL_INCLUDE_DIRS}
            ${OPENSSL_INCLUDE_DIRS}
            ${MYSQL_INCLUDE_DIRS}
            )

    # link libraries
    target_link_libraries(${test_name}
            ${openssl_libsrcs}
            mysqlclient
            pthread
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