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

namespace test_server {

    TEST(test_server, Test1) {
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
        LOG_INFO("%s", "start server");
        auto p = new ClientPool(1234, 1235, __SSLKEY__, __SSLCERT__);
        p->testing_();

    }



} // namespace cmudb
