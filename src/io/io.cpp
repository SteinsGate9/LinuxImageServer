/********************************************
 * Content: 
 * Author: by shichenh.
 * Date:  on 2020-05-18.
********************************************/

#include "io.h"


/********************************************
 * socket related
********************************************/
int open_listenfd(int port){
    /* Create socket descriptor for listening usage */
    int listenfd;
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        CONSOLE_LOG_ERROR("%s create listener socket", SOCKET_API_ERR_MSG);
    }

    /* Eliminates "Address already in use" error from bind */
    int yes = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
        CONSOLE_LOG_ERROR("%s set SO_REUSEADDR", SOCKET_API_ERR_MSG);
    }

    /* Bind listener socket */
    sockaddr_in serveraddr; memset(&serveraddr, 0, sizeof(sockaddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((uint16_t)port);

    if (bind(listenfd, (sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
        close(listenfd);
        CONSOLE_LOG_ERROR("%s bind listenser socket", SOCKET_API_ERR_MSG);
    }

    if (listen(listenfd, 5) < 0) {
        close(listenfd);
        CONSOLE_LOG_ERROR("%s listen on listener socket", SOCKET_API_ERR_MSG);
    }

    return listenfd;
}

void *get_in_addr(sockaddr *sa){
    if (sa->sa_family == AF_INET) {
        return &(((sockaddr_in *)sa)->sin_addr);
    }
    return &(((sockaddr_in6 *)sa)->sin6_addr);
}

int open_ssl_socket(int port, SSL_CTX* ssl_context) {
    int   ssl_socket;
    int yes;
    sockaddr_in addr;

    if ((ssl_socket = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        SSL_CTX_free(ssl_context);
        CONSOLE_LOG_ERROR("%s", "Failed creating socket.");
    }

    /* Eliminates "Address already in use" error from bind */
    if (setsockopt(ssl_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
        CONSOLE_LOG_ERROR("%s set SO_REUSEADDR", SOCKET_API_ERR_MSG);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(ssl_socket, (sockaddr *)&addr, sizeof(addr)) < 0)
    {
        close(ssl_socket);
        SSL_CTX_free(ssl_context);
        CONSOLE_LOG_ERROR("%s", "Failed binding SSL socket.\n");
    }

    if (listen(ssl_socket, 5) < 0) {
        close(ssl_socket);
        SSL_CTX_free(ssl_context);
        CONSOLE_LOG_ERROR("%s", "Failed listening to SSL socket.\n");
    }

    return ssl_socket;
}

/********************************************
 * epoll related
********************************************/
void print_epollevent(int event) {
    char temp[100];
    strcpy(temp, "3) epoll event: ");

    if (event & EPOLLIN) {
        strcat(temp, "EPOLLIN ");
    }
    else if (event & EPOLLOUT) {
        strcat(temp, "EPOLLOUT ");
    }
    else if (event & EPOLLRDHUP) {
        strcat(temp, "EPOLLRDHUP ");
    }
    else if (event & EPOLLPRI) {
        strcat(temp, "EPOLLPRI ");
    }
    else if (event & EPOLLERR) {
        strcat(temp, "EPOLLERR ");
    }
    else if (event & EPOLLET) {
        strcat(temp, "EPOLLET ");
    }
    else if (event & EPOLLONESHOT) {
        strcat(temp, "EPOLLET ");
    }
    else if (event & EPOLLWAKEUP) {
        strcat(temp, "EPOLLWAKEUP ");
    }
    else if (event & EPOLLHUP) {
        strcat(temp, "EPOLLHUP ");
    }

    CONSOLE_LOG_INFO("%s", temp);
}