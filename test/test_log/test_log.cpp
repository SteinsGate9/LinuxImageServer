/**
 * lru_replacer_test.cpp
 */
#include <iostream>
#include <cstdio>
#include <thread>

#include "gtest/gtest.h"

#include "logger.h"
#include "log.h"

namespace test_log {
    const char* getThreadIdOfString(const std::thread::id & id)
    {
        std::stringstream sin;
        sin << id;
        return sin.str().c_str();
    }


    void test_log(){
        LOG_INFO("thread[%d] started", std::this_thread::get_id());
        LOG_INFO("thread[%d] second message", std::this_thread::get_id());
        sleep(1);
        LOG_INFO("thread[%d] third message", std::this_thread::get_id());
        CONSOLE_LOG_INFO("thread[%s] thirdasyn message", getThreadIdOfString(std::this_thread::get_id()));
    }


    TEST(test_log, Test1) {
        ;
        /* init */
        char logpath[100];
        strcpy(logpath, __LOGPATH__);
        strcat(logpath, "/mylog.log");
        fprintf(stdout, "%s", logpath);
        bool ret0 = Log::get_instance()->init(logpath, 8192, 2000000, 0); //asyn日志模型
        EXPECT_EQ(ret0, true);

        /* go threads */
        std::vector<std::thread> threads;
        for (unsigned i = 0; i < 10000; ++i)
            threads.emplace_back(test_log);
        for (auto& i: threads)
            i.join();


    }

    TEST(test_log, Test2) {
        /* init */
        char logpath[100];
        strcpy(logpath, __LOGPATH__);
        strcat(logpath, "/mylogasyn.log");
        fprintf(stdout, "%s", logpath);
        bool ret0 = Log::get_instance()->init(logpath, 8192, 2000000, 10); //syn日志模型
        EXPECT_EQ(ret0, true);

        /* anyway */
        auto test1 = Log::get_instance();
        auto test2 = Log::get_instance();
        auto test3 = Log::get_instance();
        EXPECT_EQ(test1, test2);
        EXPECT_EQ(test2, test3);

        /* go threads */
        std::vector<std::thread> threads;
        for (unsigned i = 0; i < 10000; ++i)
            threads.emplace_back(test_log);
        for (auto& i: threads)
            i.join();
    }


} // namespace cmudb
