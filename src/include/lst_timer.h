#ifndef LST_TIMER
#define LST_TIMER

#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>

#include "log.h"

#define BUFFER_SIZE 64

class HttpConn;
class Timer{
public:
    Timer(int sockfd, sockaddr_in address, HttpConn* data, time_t expire);
    ~Timer();
    void cb_func();

public:
    HttpConn* data;
    time_t expire;
    Timer* prev, * next;
};

class SortedTimerList{
public:
    SortedTimerList() : head( NULL ), tail( NULL ) {}
    ~SortedTimerList();
    void add_timer( Timer* timer );
    void adjust_timer( Timer* timer );
    void del_timer( Timer* timer );
    void timeout();

private:
    void _add_timer( Timer* timer, Timer* lst_head );

private:
    Timer* head;
    Timer* tail;
};

#endif
