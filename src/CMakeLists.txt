#cmake_minimum_required(VERSION 3.4.1)
#project(LinuxServer)
#
## c++11
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
#
#
## Create the executable
#add_executable(${PROJECT_NAME} main2.cpp
#        CGImysql/sql_connection_pool.cpp
#        http/http_conn.cpp
#        log/log.cpp
#        lock/locker.cpp
#        timer/lst_timer.cpp
#
#        threadpool/threadpool.h
#        log/block_queue.h
#        )
#
## Including
#target_include_directories(${PROJECT_NAME} PUBLIC
#        ..
#        threadpool
#        log
#        CGImysql
#        http
#        lock
#        timer
#        )
#
## Linkng
#target_link_libraries(${PROJECT_NAME} PUBLIC
#        mysqlclient
#        pthread)



########## Set up ####################
# project name
project(project1)

# set c flags
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -lstdc++")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -Werror")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__VTableFILE__='\"$(subst ${CMAKE_SOURCE_DIR}/,,$(abspath $<))\"'")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__LOGPATH__='\"${CMAKE_SOURCE_DIR}/log/\"'")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__ROOTPATH__='\"${CMAKE_SOURCE_DIR}/root/\"'")

add_definitions(-DDEBUG_VERBOSE)  # debug

# set output paths
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)


########## Files  ####################
set(LOCAL_INCLUDE_DIRS include)
set(OPENSSL_INCLUDE_DIRS /usr/local/Cellar/openssl/1.0.2s/include)
set(MYSQL_INCLUDE_DIRS /usr/include)
set(MYSQL_LIB_DIRS /usr/lib/mysql/plugin)

file(GLOB_RECURSE c_srcs ${PROJECT_SOURCE_DIR}/*/*.c)
file(GLOB_RECURSE cpp_srcs ${PROJECT_SOURCE_DIR}/*/*.cpp
        ${PROJECT_SOURCE_DIR}/main2.cpp
        )
file(GLOB_RECURSE hpp_srcs ${PROJECT_SOURCE_DIR}/*/*.h)
file(GLOB_RECURSE mysql_lib_srcs ${MYSQL_LIB_DIRS}/*)

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





