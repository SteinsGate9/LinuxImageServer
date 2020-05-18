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

/********************************************
 * signal
********************************************/
void addsig(int sig, void(handler)(int), bool restart = true);


/********************************************
 * file system
********************************************/
int set_nonblocking(int fd);
void add_fd(int epollfd, int fd, bool one_shot, bool LT);
void remove_fd(int epollfd, int fd);
void mod_fd(int epollfd, int fd, int ev);


#endif //LINUXIMAGESERVER_COMMON_H
