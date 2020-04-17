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

#include "./lock/locker.h"
#include "./threadpool/threadpool.h"
#include "./timer/lst_timer.h"
#include "./http/http_conn.h"
#include "./log/log.h"
#include "./CGImysql/sql_connection_pool.h"
#include "define.h"

#define MAX_FD 30000           //最大文件描述符
#define MAX_EVENT_NUMBER 10000 //最大事件数
#define TIMESLOT 5             //最小超时单位

extern int add_fd(int epollfd, int fd, bool one_shot, bool LT);
extern int set_nonblocking(int fd);

static int pipefd[2];
static SortedTimerList timer_lst;

// sig handleer
static void sig_handler(int sig)
{
    //为保证函数的可重入性，保留原来的errno
    int save_errno = errno;
    int msg = sig;
    send(pipefd[1], (char *)&msg, 1, 0);
    errno = save_errno;
}

// add sig
static void add_sig(int sig, void(handler)(int), bool restart = true)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if (restart)
        sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

// timer
static void timer_handler()
{
    timer_lst.timeout();
    alarm(TIMESLOT);
}


int main(int argc, char *argv[])
{

//
#ifdef ASYNLOG
    Log::get_instance()->init("./mylog.log", 8192, 2000000, 10); //异步日志模型
#endif

#ifdef SYNLOG
    Log::get_instance()->init("./mylog.log", 8192, 2000000, 0); //同步日志模型
#endif

    int port = 1234;
    if (argc > 1)
        port = atoi(argv[1]);

    //忽略SIGPIPE信号
    add_sig(SIGPIPE, SIG_IGN);

    //单例模式创建数据库连接池
    connectionPool *connPool = connectionPool::get_instance("localhost", "root", "123", "yourdb", 3306, 8);

    //创建线程池
    ProcessThreadPool<HttpConn> *pool = NULL;
    try{
        pool = new ProcessThreadPool<HttpConn>(connPool);
    }
    catch (...){
        return 1;
    }

    HttpConn *users = new HttpConn[MAX_FD];
    assert(users);
    int user_count = 0;

#ifdef SYNSQL
    //初始化数据库读取表
    users->initmysql_result(connPool);
#endif

#ifdef CGISQLPOOL
    //初始化数据库读取表
    users->initresultFile(connPool);
#endif

    //创建套接字，返回listenfd
    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);

    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);

    // 设置端口复用，绑定端口
    int flag = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    ret = bind(listenfd, (struct sockaddr *)&address, sizeof(address));
    assert(ret >= 0);
    ret = listen(listenfd, 5);
    assert(ret >= 0);

    //创建内核事件表
    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(5);
    assert(epollfd != -1);
    add_fd(epollfd, listenfd, false, true);
    HttpConn::m_epollfd = epollfd;

    //创建管道
    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, pipefd);
    assert(ret != -1);
    set_nonblocking(pipefd[1]);
    add_fd(epollfd, pipefd[0], false, false);

    // add all the interesting signals here
    add_sig(SIGALRM, sig_handler, false);
    add_sig(SIGTERM, sig_handler, false);
    bool stop_server = false;

    // timeout
    bool timeout = false;
    alarm(TIMESLOT);

    while (!stop_server)
    {
        int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if (number < 0 && errno != EINTR)
        {
            LOG_ERROR("%s", "epoll failure");
            break;
        }

        for (int i = 0; i < number; i++)
        {
            printf("get\n");
            int sockfd = events[i].data.fd;

            //处理新到的客户连接
            if (sockfd == listenfd)
            {
                struct sockaddr_in client_address;
                socklen_t client_addrlength = sizeof(client_address);
                int connfd = accept(listenfd, (struct sockaddr *)&client_address, &client_addrlength);
                if (connfd < 0)
                {
                    LOG_ERROR("%s:errno is:%d", "accept error", errno);
                    continue;
                }
                if (HttpConn::m_user_count >= MAX_FD)
                {
                    send(connfd, "Internal server busy", strlen("Internal server busy"), 0);
                    close(connfd);
                    LOG_ERROR("%s", "Internal server busy");
                    continue;
                }
                Timer *timer = new Timer(connfd, client_address, &users[connfd], time(NULL)+3*TIMESLOT);
                timer_lst.add_timer(timer);

                users[connfd].init(connfd, client_address, timer);
            }

            else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                users[sockfd].close_conn();

                Timer *timer = users[sockfd].timer;
                if (timer){
                    timer_lst.del_timer(timer);
                }
            }

            //处理信号
            else if ((sockfd == pipefd[0]) && (events[i].events & EPOLLIN))
            {
                int sig;
                char signals[1024];
                ret = recv(pipefd[0], signals, sizeof(signals), 0);
                if (ret == -1)
                {
                    // handle the error
                    continue;
                }
                else if (ret == 0)
                {
                    continue;
                }
                else
                {
                    for (int i = 0; i < ret; ++i)
                    {
                        switch (signals[i])
                        {
                        case SIGALRM:
                        {
                            timeout = true;
                            break;
                        }
                        case SIGTERM:
                        {
                            stop_server = true;
                        }
                        }
                    }
                }
            }

            //处理客户连接上接收到的数据
            else if (events[i].events & EPOLLIN)
            {
                Timer *timer = users[sockfd].timer;
                if (users[sockfd].read_once())
                {
                    LOG_INFO("deal with the client(%s)", inet_ntoa(users[sockfd].get_address()->sin_addr));
                    Log::get_instance()->flush();
                    //若监测到读事件，将该事件放入请求队列
                    pool->append(users + sockfd);

                    //若有数据传输，则将定时器往后延迟3个单位
                    //并对新的定时器在链表上的位置进行调整
                    if (timer)
                    {
                        time_t cur = time(NULL);
                        timer->expire = cur + 3 * TIMESLOT;
                        LOG_INFO("%s", "adjust timer once");
                        Log::get_instance()->flush();
                        timer_lst.adjust_timer(timer);
                    }
                }
                else
                {
                    users[sockfd].close_conn();
                    if (timer)
                    {
                        timer_lst.del_timer(timer);
                    }
                }
            }
            else if (events[i].events & EPOLLOUT)
            {

                if (!users[sockfd].write())
                {
                    users[sockfd].close_conn();
                }
            }
        }
        if (timeout)
        {
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
