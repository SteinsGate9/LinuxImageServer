/********************************************
 * Content: 
 * Author: by shichenh.
 * Date:  on 2020-05-18.
********************************************/
#include "common.h"

/********************************************
 * sighandlers
********************************************/
void addsig(int sig, void(handler)(int), bool restart) {
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if (restart)
        sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}


/********************************************
 * file systems
********************************************/
int set_nonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void set_fl(int fd, int flags)
{
    int val;

    if ((val = fcntl(fd, F_GETFL, 0)) < 0){
        CONSOLE_LOG_ERROR("fcntl F_GETFL error with errno %d", errno);
    }

    val |= flags;		/* turn on flags */

    if (fcntl(fd, F_SETFL, val) < 0) {
        CONSOLE_LOG_ERROR("fcntl F_SETFL error with errno %d", errno);
    }
}

void clr_fl(int fd, int flags)

{
    int val;

    if ((val = fcntl(fd, F_GETFL, 0)) < 0) {
        CONSOLE_LOG_ERROR("fcntl F_GETFL error with errno %d", errno);
    }

    val &= ~flags;		/* turn flags off */

    if (fcntl(fd, F_SETFL, val) < 0) {
        CONSOLE_LOG_ERROR("fcntl F_SETFL error with errno %d", errno);
    }
}

void add_fd(int epollfd, int fd, bool one_shot, bool LT)
{
    epoll_event event;
    event.data.fd = fd;
    if (LT) {
        event.events = EPOLLIN | EPOLLRDHUP;
    }
    else{
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
        set_fl(fd, O_NONBLOCK);
    }
    if (one_shot)
        event.events |= EPOLLONESHOT;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event) < 0){
        CONSOLE_LOG_ERROR("epoll_ctl error: %s", strerror(errno));
    }
}


void remove_fd(int epollfd, int fd)
{
    if (epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0) < 0){
        CONSOLE_LOG_ERROR("epoll_ctl error: %s", strerror(errno));
    }
    if (close(fd) <= 1){
        CONSOLE_LOG_ERROR("close error: %s", strerror(errno));
    }
}


void mod_fd(int epollfd, int fd, int ev)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}


/********************************************
 * mem
********************************************/
//void unmap(char* file_address, ){
//    if (file_address){
//        munmap(file_address, m_file_stat.st_size);
//        file_address = 0;
//    }
//}