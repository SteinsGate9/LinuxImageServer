/**
 * lru_replacer_test.cpp
 */
#include <iostream>
#include <cstdio>
#include <thread>
#include <map>

#include "gtest/gtest.h"

#include "http_conn.h"
#include "logger.h"

namespace test_log {
    void test_log(){
        LOG_INFO("thread[%d] started", std::this_thread::get_id());
        LOG_INFO("thread[%d] second message", std::this_thread::get_id());
        sleep(1);
        LOG_INFO("thread[%d] third message", std::this_thread::get_id());
        CONSOLE_LOG_INFO("thread[%d] thirdasyn message", std::this_thread::get_id());
    }


    TEST(test_mysql, Test1) {
        HttpHandler::initmysql_result();
        EXPECT_EQ(HttpHandler::users.count("root"), 1);
        EXPECT_EQ(HttpHandler::users["root"], "123456");
    }

    TEST(test_mysql, Test2) {
        HttpHandler::initmysql_result();
        EXPECT_EQ(HttpHandler::users.count("root"), 1);
        EXPECT_EQ(HttpHandler::users["root"], "123456");
    }



} // namespace cmudb
