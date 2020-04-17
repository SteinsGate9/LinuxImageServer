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

#include "threadpool/threadpool.h" // connect.h locker.h
#include "CGImysql/sql_connection_pool.h" // locker.h
#include "timer/lst_timer.h" // http.h log.h
#include "http/http_conn.h" // log.h connect.h locker.h
#include "log/block_queue.h"
#include "define.h"

#define MAX_FD 30000           //最大文件描述符
#define MAX_EVENT_NUMBER 10000 //最大事件数
#define TIMESLOT 5             //最小超时单位

extern int add_fd(int epollfd, int fd, bool one_shot, bool LT);
extern int set_nonblocking(int fd);

static int pipefd[2];
static SortedTimerList timer_lst;

// sig
static void sig_handler(int sig){
    int save_errno = errno;
    int msg = sig;
    send(pipefd[1], (char *)&msg, 1, 0);
    errno = save_errno;
}

// set sig
static void addsig(int sig, void(handler)(int), bool restart = true) {
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if (restart)
        sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

// time handler
static void timer_handler(){
    timer_lst.timeout();
    alarm(TIMESLOT);
}


int main(int argc, char *argv[])
{
/*deal with log*/
#ifdef ASYNLOG
    bool ret = Log::get_instance()->init("./mylog.log", 8192, 2000000, 10); //异步日志模型
#endif
#ifdef SYNLOG
    bool ret0 = Log::get_instance()->init("./mylog.log", 8192, 2000000, 0); //同步日志模型
#endif
    if(!ret0) PRINTERROR("ERROR: creating logging file failed");


/*deal with input info*/
    int port = 1234;
    if (argc == 2) port = atoi(argv[1]);


// todo: ignore SIGPIPE?
    addsig(SIGPIPE, SIG_IGN);


/*epoll FDs */
    //  epoll events
    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(5);
    if(epollfd == -1) PRINTERROR("ERROR: epoll create failed");


/*threadpool ialization*/
    // singeliton mysql pool
    connectionPool *connPool = connectionPool::get_instance("localhost", "root", "123", "yourdb", 3306, 8);

    // threadpool
    ProcessThreadPool<HttpConn> *pool = NULL;
    pool = new ProcessThreadPool<HttpConn>(connPool);
    if(!pool) PRINTERROR("ERROR: create new threadpool failed");

    // all threadpool items
    HttpConn *users = new HttpConn[MAX_FD];
    if(!users) PRINTERROR("ERROR: create users HTTP items failed");

    //  connpoll for HTTP
#ifdef SYNSQL
    users->initmysql_result(connPool); // init FD mysql conn
#endif
#ifdef get_POOL
    users->initresultFile(connPool);
#endif

    // init epollfd for HTTP
    HttpConn::m_epollfd = epollfd;


/*listen socket initialization & add to FD*/
    // init listen socket
    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    if(listenfd < 0) PRINTERROR("ERROR: create users HTTP failed");

    // set address for listen socket
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);

    // bind address to listen socket & set SO_REUSEADDR
    int flag = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    int ret = bind(listenfd, (struct sockaddr *)&address, sizeof(address));
    if(ret < 0) PRINTERROR("ERROR: binding failed");
    ret = listen(listenfd, 5);
    if(ret < 0) PRINTERROR("ERROR: listening failed");

    // add to epoll
    add_fd(epollfd, listenfd, false, true); // add listen fd to epoll.


/*signal pipe & add to FD*/
    // create PIPE
    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, pipefd); // pipefd are fds for 2 sockets,
    // fd[1] for sending, fd[0] for receiving.
    if(ret == -1) PRINTERROR("ERROR: create PIPE failed");
    set_nonblocking(pipefd[1]);

    // add to epoll
    add_fd(epollfd, pipefd[0], false, false); // add signal fd to epoll


/*timer list*/
    // register time out signal here
    addsig(SIGALRM, sig_handler, false);
    addsig(SIGTERM, sig_handler, false);

    // start timer
    bool timeout = false;
    alarm(TIMESLOT);


/*epoll main thread*/
    printf("start listening\n");
    bool stop_server = false;
    while (!stop_server)
    {
        int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);

        // epoll failed
        if (number < 0 && errno != EINTR){
            LOG_ERROR("%s", "epoll failure");
            break;
        }

        // epoll succeeded
        for (int i = 0; i < number; i++){
            int sockfd = events[i].data.fd; // check fd

            if (sockfd == listenfd){ // build TCP
                struct sockaddr_in client_address;
                socklen_t client_addrlength = sizeof(client_address);
                int connfd = accept(listenfd, (struct sockaddr *)&client_address, &client_addrlength); // try TCP connect
                if (connfd < 0){
                    LOG_ERROR("%s:errno is:%d", "accept error", errno);
                    PRINTERROR("ERROR: creating TCP connection");
                    continue;
                }
                if (HttpConn::m_user_count >= MAX_FD){ // undealt HTTP >= 65532
                    send(connfd, "Internal server busy", strlen("Internal server busy"), 0);
                    close(connfd);
                    LOG_ERROR("ERROR: Internal server busy: %s", errno);
                    PRINTERROR("ERROR: Internal server busy");
                    continue;
                }
                LOG_INFO("fd=%d, listen=%d, sig=%d, number=%d, idx=listen %d", sockfd, listenfd, pipefd[0], number, connfd);
                // add timer
                Timer *timer = new Timer(connfd, client_address, &users[connfd], time(NULL)+3*TIMESLOT);
                timer_lst.add_timer(timer); // add TIMER to TIMERLIST

                // init client_data
                users[connfd].init(connfd, client_address, timer); // init HTTPCON
            }

            else if ((sockfd == pipefd[0]) && (events[i].events & EPOLLIN)){ // signal

                int sig;
                char signals[1024];
                ret = recv(pipefd[0], signals, sizeof(signals), 0); // recv signal

                if (ret == -1) continue; // todo: handle the error
                else if (ret == 0) continue;
                else{
                    for (int i = 0; i <= ret-1; ++i){
                        LOG_INFO("fd=%d, listen=%d, sig=%d, number=%d, idx=signal %d", sockfd, listenfd, pipefd[0], number, signals[i]);
                        switch (signals[i]){
                            case SIGALRM:{ // receive timeout then set timeout = True
                                timeout = true;
                                break;
                            }
                            case SIGTERM: stop_server = true; // receive KILL
                        }
                    }
                }
            }

            else if (events[i].events & EPOLLIN){ // read data
                LOG_INFO("fd=%d, listen=%d, sig=%d, number=%d, idx=readdata", sockfd, listenfd, pipefd[0], number);
                Timer *timer = users[sockfd].timer;
                if (users[sockfd].read_once()){
                    LOG_INFO("deal with the client(%s)", inet_ntoa(users[sockfd].get_address()->sin_addr));
                    pool->append(users + sockfd); //若监测到读事件，将该事件放入请求队列

                    if (timer){
                        timer->expire = (time(NULL) + 3 * TIMESLOT); //若有数据传输，则将定时器往后延迟3个单位
                        timer_lst.adjust_timer(timer); //并对新的定时器在链表上的位置进行调整
                        LOG_INFO("%s", "adjust timer once");
                    }
                }
                else{
                    users[sockfd].close_conn();
                    if (timer) timer_lst.del_timer(timer);
                }
            }

            else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)){ // failed
                LOG_INFO("fd=%d, listen=%d, sig=%d, number=%d, idx=failed", sockfd, listenfd, pipefd[0], number);
                users[sockfd].close_conn();
                Timer *timer = users[sockfd].timer;
                if (timer) timer_lst.del_timer(timer);
            }

            else if (events[i].events & EPOLLOUT) { // send data
                LOG_INFO("fd=%d, listen=%d, sig=%d, number=%d, idx=send", sockfd, listenfd, pipefd[0], number);
                if (!users[sockfd].write()) users[sockfd].close_conn();
            }

        }
        if (timeout){
            timer_handler();
            timeout = false;
        }
    }
    close(epollfd);
    close(listenfd);
    close(pipefd[1]);
    close(pipefd[0]);
    delete[] users;
    delete pool;
    delete connPool;

    return 0;
}
