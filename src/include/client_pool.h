/********************************************
*   Content:
*   Author: shichenh
*   Date Created: May 18, 2020
********************************************/
#ifndef LISO_SERVER_H
#define LISO_SERVER_H

#include "io.h"
#include "logger.h"
#include "common.h"
#include "http_conn.h"
#include "threadpool.h"
#include "define.h"
#include "connector.h"
#include "ssl.h"


/********************************************
 * defines
********************************************/
#define MAX_FD 30000           //最大文件描述符
#define MAX_SIG 10             //最大文件描述符
#define MAX_EVENT_NUMBER 10000 //最大事件数
#define TIMESLOT 5             //最小超时单位


/********************************************
 * types
********************************************/
enum class ServerState {
    STOPPED,
    RUNNING,
    SLEEPING
} ;


/********************************************
 * class
********************************************/
class ClientPool{
public:
    explicit ClientPool(int, int, char*, char*);
    ~ClientPool(){
        close(epollfd_);
        close(listenfd_);
        close(alarmfd[1]);
        close(alarmfd[0]);
        close(intfd[1]);
        close(intfd[0]);
        delete httppool_;
    }
    /* handle event for server */
    void handle_events();

    /* testing handling problems:
     * 1) if timeout comes first then read => can't happen
     * 2) if timeout not finished then read => deal within httphandler
     * 3) if timeout finished then read => can't happen  */
    void testing_();

private:
    /* signal handler*/
    static void sig_handler(int sig){

        int save_errno = errno;
        int msg = sig;
        if (msg == SIGALRM) {
            send(alarmfd[1], (char *) &msg, 1, 0);
        }
        else {
            send(intfd[1], (char *) &msg, 1, 0);
        }
        errno = save_errno;
    }

    /* add to client */
    void add_client_();
    void add_ssl_client_();

public:
    /* server state */
    ServerState server_state;

private:
    /* epoll related */
    int epollfd_;
    epoll_event events_[MAX_EVENT_NUMBER]; // events for storing

    /* normal related */
    int listenfd_;

    /* ssl related */
    int ssl_fd_;
    SSL_CTX *ssl_context_;

    /* http thread pools related */
    SortedTimerList timer_lst_; /* control the process of pool. */
    HttpHandler users_[MAX_FD]; /* all possible users */
    ProcessThreadPool<HttpHandler> *httppool_; /* continously append user to pool */

    /* signal handler pools related */
    static int alarmfd[2]; /*  pipe for signal */
    static int intfd[2];

    /* SSL related */
//    client_type type[FD_SETSIZE];                 /* client's type: HTTP or HTTPS */
//    SSL * context[FD_SETSIZE];                    /* set if client's type is HTTPS */

    /* CGI related */
    int cgi_client[FD_SETSIZE];








private:

} ;




#endif //LISO_SERVER_H