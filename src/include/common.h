/********************************************
 * Content: 
 * Author: by shichenh.
 * Date:  on 2020-05-18.
********************************************/
#ifndef LINUXIMAGESERVER_COMMON_H
#define LINUXIMAGESERVER_COMMON_H


#include <sys/epoll.h>
#include <unistd.h>
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
#include <signal.h>
#include <string.h>

#include "logger.h"


/********************************************
 * signal
********************************************/
// set sig
void addsig(int sig, void(handler)(int), bool restart = true);


/********************************************
 * file system
********************************************/
//对文件描述符设置非阻塞
int set_nonblocking(int fd);

/* flags are file status flags to turn on */
void set_fl(int fd, int flags);

/* flags are the file status flags to turn off */
void clr_fl(int fd, int flags);

//将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
void add_fd(int epollfd, int fd, bool one_shot, bool LT);

//从内核时间表删除描述符
void remove_fd(int epollfd, int fd);

//将事件重置为EPOLLONESHOT
void mod_fd(int epollfd, int fd, int ev);

/********************************************
 * mem
********************************************/
void unmap();


#endif //LINUXIMAGESERVER_COMMON_H
