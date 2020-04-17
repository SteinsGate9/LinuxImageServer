cmake_minimum_required(VERSION 3.4.1)
project(LinuxServer)

# c++11
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")

# Create the executable
add_executable(${PROJECT_NAME} main.cpp
        CGImysql/sql_connection_pool.cpp
        http/http_conn.cpp
        log/log.cpp
        lock/locker.cpp
        timer/lst_timer.cpp

        threadpool/threadpool.h
        log/block_queue.h
        )

# Including
target_include_directories(${PROJECT_NAME} PUBLIC
        threadpool
        log
        CGImysql
        http
        lock
        timer
        )

# Linkng
target_link_libraries(${PROJECT_NAME} PUBLIC
        mysqlclient
        pthread)








