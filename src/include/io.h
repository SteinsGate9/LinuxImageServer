/********************************************
 * Content: 
 * Author: by shichenh.
 * Date:  on 2020-05-18.
********************************************/
#ifndef LINUXIMAGESERVER_IO_H
#define LINUXIMAGESERVER_IO_H

#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <netinet/in.h>
#include <ctype.h>
#include <time.h>
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
#include <string.h>
#include <openssl/ssl.h>

#include "logger.h"

/********************************************
 * define
********************************************/
#define SOCKET_API_ERR_MSG "[Error in socket_api]"


/********************************************
 * sockets
********************************************/
int open_listenfd(int port);
void *get_in_addr(sockaddr *sa);
int open_ssl_socket(int port, SSL_CTX* ssl_context);


/********************************************
 * epoll
********************************************/
void print_epollevent(int event);




#endif //LINUXIMAGESERVER_IO_H
