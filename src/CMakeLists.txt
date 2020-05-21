########## Set up ####################
# project name
project(project1)

# set system
string(REGEX MATCH "Linux"  IS_LINUX "${CMAKE_SYSTEM_NAME}")
string(REGEX MATCH "Darwin" IS_MACOS "${CMAKE_SYSTEM_NAME}")

# set c flags
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -lstdc++")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -Werror")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__VTableFILE__='\"$(subst ${CMAKE_SOURCE_DIR}/,,$(abspath $<))\"'")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__LOGFILE__='\"${CMAKE_SOURCE_DIR}/log/mylog.log\"'")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__ROOTPATH__='\"${CMAKE_SOURCE_DIR}/root\"'")
add_definitions(-DDEBUG_VERBOSE)  # debug

# set output paths
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)


########## Files  ####################
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

file(GLOB_RECURSE c_srcs ${PROJECT_SOURCE_DIR}/*/*.c)
file(GLOB_RECURSE cpp_srcs ${PROJECT_SOURCE_DIR}/*/*.cpp
        ${PROJECT_SOURCE_DIR}/main2.cpp
        )
file(GLOB_RECURSE hpp_srcs ${PROJECT_SOURCE_DIR}/*/*.h)


########## Executables ####################
# add executables
add_executable(liso
        ${c_srcs}
        ${cpp_srcs}
        ${hpp_srcs}
        )

# include dirs
target_include_directories(liso PUBLIC
        ${LOCAL_INCLUDE_DIRS}
        ${OPENSSL_INCLUDE_DIRS}
        ${MYSQL_INCLUDE_DIRS}
        )
target_link_libraries(liso PUBLIC
        mysqlclient
        pthread
        )





