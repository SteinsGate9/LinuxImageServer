/********************************************
 * Content: 
 * Author: by shichenh.
 * Date:  on 2020-05-19.
********************************************/

#ifndef LINUXIMAGESERVER_CONNECTOR_H
#define LINUXIMAGESERVER_CONNECTOR_H

#include "http_conn.h"
#include "lst_timer.h"
#include "io.h"


#define MAX_FD_ 30000


class ConnectHandler{
public:
    void process(){
        std::unique_lock<mutex> m_lock(mutex_);

        /* build TCP */
        struct sockaddr_in client_address;
        socklen_t client_addrlength = sizeof(client_address);
        int connfd; /* try tcp */
        if ((connfd = accept(listenfd, (struct sockaddr *) &client_address, &client_addrlength)) < 0) {
#ifdef DEBUG_VERBOSE
            CONSOLE_LOG_WARN("%s", "epoll failure");
#endif
            LOG_ERROR("%s:errno is:%d", "accept error", errno);
        }
        m_lock.unlock();

        /* get address */
#ifdef DEBUG_VERBOSE
        char remoteIP[INET6_ADDRSTRLEN];
        char *remote_addr = (char *) inet_ntop(client_address.sin_family,
                                               get_in_addr((sockaddr *) &client_address), remoteIP,
                                               INET6_ADDRSTRLEN);
        CONSOLE_LOG_INFO("address: %s", remote_addr);
#endif

        /* if too much */
        if (HttpHandler::m_user_count == MAX_FD_){ // undealt HTTP >= 65532
            send(connfd, "Internal server busy\r\n", strlen("Internal server busy\r\n"), 0);
            close(connfd);
            CONSOLE_LOG_WARN("ERROR: Internal server busy");
            LOG_ERROR("ERROR: Internal server busy: %s", errno);
        }


        /* add to time list */
        m_lock.lock();
        Timer *timer = new Timer(connfd, client_address, &users[connfd], time(NULL)+3*5);
        timer_lst->add_timer(timer); // add TIMER to TIMERLIST

        /* add to thread pool */
        users[connfd].init(connfd, client_address, timer, CLIENT_TYPE::HTTP_CLIENT, nullptr); // init HTTPCON
        m_lock.unlock();
    };

public:
    static SortedTimerList* timer_lst;
    static HttpHandler* users;
    static int listenfd;
    MYSQL* mysql;

private:
    std::mutex mutex_;


};


#endif //LINUXIMAGESERVER_CONNECTOR_H
