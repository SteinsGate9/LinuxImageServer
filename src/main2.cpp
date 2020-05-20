//
// Created by huangbenson on 4/15/20.
//

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <sys/epoll.h>

#include "threadpool.h" // connect.h locker.h
#include "sql_connection_pool.h" // locker.h
#include "lst_timer.h" // http.h log.h
#include "http_conn.h" // log.h connect.h locker.h
#include "block_queue.h"
#include "log.h"
#include "define.h"
#include "logger.h"
#include "common.h"
#include "io.h"
#include "client_pool.h"


int main(int argc, char *argv[]){
    /* port */
    int port = 1234;
    if (argc == 2)
        port = strtol(argv[1], nullptr, 10);

    /* init log */
#ifdef SYNLOG
    Log::get_instance()->init(__LOGFILE__, 8192, 2000000, 0); //同步日志模型
#endif
#ifdef ASYNLOG
    Log::get_instance()->init(__LOGFILE__, 8192, 2000000, 10); //异步日志模型
#endif

    /* init mysql result */
#ifdef SYNSQL
    HttpHandler::initmysql_result(); // init FD mysql conn
#endif
#ifdef get_POOL
    users->initresultFile();
#endif

    /* client pool */
    auto *clientpool = new ClientPool(port);

    /* start */
    CONSOLE_LOG_INFO("-----[LISO] start-----");
    LOG_INFO("%s", "-----[LISO] start-----");
    clientpool->handle_events();



    return 0;
}
