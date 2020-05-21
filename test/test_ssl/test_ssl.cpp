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
#include "client_pool.h"
#include "ssl.h"


namespace test_epoll {
    void test_log(){
        LOG_INFO("thread[%d] started", std::this_thread::get_id());
        LOG_INFO("thread[%d] second message", std::this_thread::get_id());
        sleep(1);
        LOG_INFO("thread[%d] third message", std::this_thread::get_id());
//        CONSOLE_LOG_INFO("thread[%d] thirdasyn message", std::this_thread::get_id());
    }



    TEST(test_epoll, Test1) {
        /* init log */
#ifdef SYNLOG
        Log::get_instance()->init(__LOGFILE__, 8192, 2000000, 0); //同步日志模型
#endif
#ifdef ASYNLOG
        Log::get_instance()->init(__LOGFILE__, 8192, 2000000, 10); //异步日志模型
#endif

        /* clear socket */
        system("/tmp/tmp.h7Dy0SaBof/test/test_epoll/close_sock.sh");
        char temp[100];
        string path = getcwd(temp, sizeof(temp)) ? std::string( temp ) : std::string("");
        cout<< path <<endl;

        /* test */
        for(int i = 0; i<=9; i++) strcat(correct2, correct);

        /* ssl */
        auto p = new ClientPool(1234, 1235, __SSLKEY__, __SSLCERT__);
        p->testing_();


    }



} // namespace cmudb
