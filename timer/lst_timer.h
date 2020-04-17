#ifndef LST_TIMER
#define LST_TIMER

#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include "../log/log.h"

#define BUFFER_SIZE 64

class http_conn;
class util_timer{
public:
    util_timer(int sockfd, sockaddr_in address, http_conn* data, time_t expire);
    ~util_timer();
    void set_expire(time_t expire_);
    void cb_func();

public:
//    sockaddr_in address;  // todo: change this
//    int sockfd;
    http_conn* data;
    time_t expire;
    util_timer* prev, * next;
};

class sort_timer_lst{
public:
    sort_timer_lst() : head( NULL ), tail( NULL ) {}
    ~sort_timer_lst();
    void add_timer( util_timer* timer );
    void adjust_timer( util_timer* timer );
    void del_timer( util_timer* timer );
    void timeout();

private:
    void _add_timer( util_timer* timer, util_timer* lst_head );

private:
    util_timer* head;
    util_timer* tail;
};

#endif
