/********************************************
 * Content: 
 * Author: by shichenh.
 * Date:  on 2020-05-18.
********************************************/
#include "common.h"

/********************************************
 * sighandlers
********************************************/
// set sig
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
//对文件描述符设置非阻塞
int set_nonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

//将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
void add_fd(int epollfd, int fd, bool one_shot, bool LT)
{
    epoll_event event;
    event.data.fd = fd;
    if (LT) {
        event.events = EPOLLIN | EPOLLRDHUP;
    }
    else{
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
        set_nonblocking(fd);
    }
    if (one_shot)
        event.events |= EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);

}

//从内核时间表删除描述符
void remove_fd(int epollfd, int fd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

//将事件重置为EPOLLONESHOT
void mod_fd(int epollfd, int fd, int ev)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}
