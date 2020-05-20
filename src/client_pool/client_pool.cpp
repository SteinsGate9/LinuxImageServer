/********************************************
 * Content: 
 * Author: by shichenh.
 * Date:  on 2020-05-18.
********************************************/
#include "client_pool.h"


/********************************************
 * static
********************************************/
int ClientPool::intfd[];
int ClientPool::alarmfd[];


/********************************************
 * functions
********************************************/
/* Initialize client pool */
ClientPool::ClientPool(int port)
: httppool_(new ProcessThreadPool<HttpHandler>()),
//  connpool_(new ProcessThreadPool<ConnectHandler>()),
  server_state(ServerState::RUNNING){
    /* epoll event */
    if((epollfd_ = epoll_create(5)) == -1) {
        LOG_ERROR("%s", "open epoll failed");
#ifdef DEBUG_VERBOSE
        CONSOLE_LOG_ERROR("epoll create failed");
#endif
    }
    memset(events_, '\0', sizeof(events_));

    /* init listen socket */
    if ((listenfd_ = open_listenfd(port)) == -1) {
        LOG_ERROR("%s", "open listenfd failed");
#ifdef DEBUG_VERBOSE
        CONSOLE_LOG_ERROR("open listenfd failed");
#endif
    }
    /* add to epoll */
    add_fd(epollfd_, listenfd_, false, true);

    /* create signal pipe */
    int ret; /* pipefd are fds for 2 sockets fd[1] for sending, fd[0] for receiving. */
    if((ret = socketpair(PF_UNIX, SOCK_STREAM, 0, alarmfd)) == -1) {
        LOG_ERROR("%s", "create pipe failed");
#ifdef DEBUG_VERBOSE
        CONSOLE_LOG_ERROR("create pipe failed");
#endif
    }
    set_fl(alarmfd[1], O_NONBLOCK);
    /* add to epoll */
    add_fd(epollfd_, alarmfd[0], false, false);
    /* create signal pipe */
    if((ret = socketpair(PF_UNIX, SOCK_STREAM, 0, intfd)) == -1) {
        LOG_ERROR("%s", "create pipe failed");
#ifdef DEBUG_VERBOSE
        CONSOLE_LOG_ERROR("create pipe failed");
#endif
    }
    set_fl(intfd[1], O_NONBLOCK);
    /* add to epoll */
    add_fd(epollfd_, intfd[0], false, false);

    /* register sighandler */
    addsig(SIGALRM, sig_handler, false);
    addsig(SIGTERM, sig_handler, false);
    addsig(SIGPIPE, SIG_IGN); // todo: why?

    /* start timer */
    alarm(TIMESLOT);

    /* init http handler */
    HttpHandler::m_epollfd = epollfd_;
}




void ClientPool::testing_(){
    bool timeout = false;
    CONSOLE_LOG_INFO("start server");

    while (server_state == ServerState::RUNNING) {
#ifdef DEBUG_VERBOSE
        fprintf(stdout, "\n");
#endif
        /* try epoll */
        int number;
        if ((number = epoll_wait(epollfd_, events_, MAX_EVENT_NUMBER, -1)) < 0) { // errno not set while -1
            if (errno == EBADF || errno == EFAULT || errno == EINVAL) {
                LOG_ERROR("%s with error %s", "epoll failure", strerror(errno));
#ifdef DEBUG_VERBOSE
                CONSOLE_LOG_ERROR("%s with error %s", "epoll failure", strerror(errno));
#endif
            }
            if (errno == EINTR) { /* signal handler will trigger this condition */
                continue;
            }
        }
#ifdef DEBUG_VERBOSE
        CONSOLE_LOG_INFO("1) epoll_wait get %d events", number);
#endif

        struct sockaddr_in client_address;
        socklen_t client_addrlength = sizeof(client_address);
        char remoteIP[INET6_ADDRSTRLEN];
        for (int i = 0; i < number; i++) {

            /* get event fd */
            int sockfd = events_[i].data.fd;
            int sockevent = events_[i].events;
#ifdef DEBUG_VERBOSE
            CONSOLE_LOG_INFO("2) fd: %d", sockfd);
            print_epollevent(sockevent);
#endif
            /* if from listen */
            if (sockfd == listenfd_) {
                add_client_();
            }
            /* signal caught */
            else if (sockfd == alarmfd[0]) { /*TODO: what if timeout after read? */
                timer_lst_.timeout();
                alarm(TIMESLOT);
            }
            else if (sockfd == intfd[0]) {
                server_state = ServerState::STOPPED;
            }
            /* other connfd */
            else {
                if (sockevent & EPOLLIN) { /* connfd have sent data */
                    /* adjust timer */
                    Timer *timer = users_[sockfd].adjust_timer();
                    timer_lst_.adjust_timer(timer);
                    /* append to work */
                    httppool_->append(users_ + sockfd);
                }
                else if (sockevent & EPOLLOUT) { /* connfd have data to send */
#ifdef DEBUG_VERBOSE
                    CONSOLE_LOG_WARN("FUCKED");
#endif
                }
                else if (sockevent & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) { /* aka fucked */
#ifdef DEBUG_VERBOSE
                    CONSOLE_LOG_WARN("FUCKED");
#endif
                    users_[sockfd].close();
                    Timer *timer = users_[sockfd].m_timer;
                    timer_lst_.del_timer(timer);
                }
            }
        }
    }
    if (server_state == ServerState::SLEEPING){
        while(1){}
    }
}


//void ClientPool::handle_events(){
//    int ret = -1;
//    bool stop_server = false;
//    bool timeout = false;
//    while (!stop_server) {
//        /* try epoll */
//        int number;
//        if ((number = epoll_wait(epollfd_, events_, MAX_EVENT_NUMBER, -1)) < 0 && errno != EINTR){
//            CONSOLE_LOG_ERROR("%s", "epoll failure");
//        }
//
//        /* go through epoll */
//        for (int i = 0; i < number; i++) {
//            int sockfd = events_[i].data.fd; // check fd
//
//
//            if (sockfd == listenfd_){ // build TCP
//                struct sockaddr_in client_address;
//                socklen_t client_addrlength = sizeof(client_address);
//                int connfd; // try TCP connect
//                if ((connfd = accept(listenfd_, (struct sockaddr *)&client_address, &client_addrlength)) < 0){
//                    CONSOLE_LOG_WARN("%s", "epoll failure");
//                    LOG_ERROR("%s:errno is:%d", "accept error", errno);
//                    continue;
//                }
//                if (HttpHandler::m_user_count >= MAX_FD){ // undealt HTTP >= 65532
//                    send(connfd, "Internal server busy", strlen("Internal server busy"), 0);
//                    close(connfd);
//                    CONSOLE_LOG_WARN("ERROR: Internal server busy");
//                    LOG_ERROR("ERROR: Internal server busy: %s", errno);
//                    continue;
//                }
//
//
//                // add timer
//                Timer *timer = new Timer(connfd, client_address, &users_[connfd], time(NULL)+3*TIMESLOT);
//                timer_lst_.add_timer(timer); // add TIMER to TIMERLIST
//
//                // init client_data
//                users_[connfd].init(connfd, client_address, timer); // init HTTPCON
//            }
//
//            else if ((sockfd == alarmfd[0]) && (events_[i].events & EPOLLIN)){ // signal
//
////                int sig;
//                char signals[1024];
//                ret = recv(alarmfd[0], signals, sizeof(signals), 0); // recv signal
//
//                if (ret == -1) continue; // todo: handle the error
//                else if (ret == 0) continue;
//                else{
//                    for (int i = 0; i <= ret-1; ++i){
//                        LOG_INFO("fd=%d, listen=%d, sig=%d, number=%d, idx=signal %d", sockfd, listenfd_, alarmfd[0], number, signals[i]);
//                        switch (signals[i]){
//                            case SIGALRM:{ // receive timeout then set timeout = True
//                                timeout = true;
//                                break;
//                            }
//                            case SIGTERM: stop_server = true; // receive KILL
//                        }
//                    }
//                }
//            }
//
//            else if (events_[i].events & EPOLLIN){ // read data
//                LOG_INFO("fd=%d, listen=%d, sig=%d, number=%d, idx=readdata", sockfd, listenfd_, alarmfd[0], number);
//                Timer *timer = users_[sockfd].m_timer;
//                if (users_[sockfd].read()){
//                    LOG_INFO("deal with the client(%s)", inet_ntoa(users_[sockfd].get_address()->sin_addr));
//                    httppool_->append(users_ + sockfd); //若监测到读事件，将该事件放入请求队列
//
//                    if (timer){
//                        timer->expire = (time(NULL) + 3 * TIMESLOT); //若有数据传输，则将定时器往后延迟3个单位
//                        timer_lst_.adjust_timer(timer); //并对新的定时器在链表上的位置进行调整
//                        LOG_INFO("%s", "adjust timer once");
//                    }
//                }
//                else{
//                    users_[sockfd].close();
//                    if (timer) timer_lst_.del_timer(timer);
//                }
//            }
//
//            else if (events_[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)){ // failed
//                LOG_INFO("fd=%d, listen=%d, sig=%d, number=%d, idx=failed", sockfd, listenfd_, alarmfd[0], number);
//                users_[sockfd].close();
//                Timer *timer = users_[sockfd].m_timer;
//                if (timer) timer_lst_.del_timer(timer);
//            }
//
//            else if (events_[i].events & EPOLLOUT) { // send data
//                LOG_INFO("fd=%d, listen=%d, sig=%d, number=%d, idx=send", sockfd, listenfd_, alarmfd[0], number);
//                if (!users_[sockfd].write()) users_[sockfd].close();
//            }
//
//        }
//        if (timeout){
//            timer_lst_.timeout();
//            alarm(TIMESLOT);
//            timeout = false;
//        }
//    }
//}

void ClientPool::add_client_() {
    /* build TCP */
    struct sockaddr_in client_address;
    socklen_t client_addrlength = sizeof(client_address);
    int connfd; /* try tcp */
    if ((connfd = accept(listenfd_, (struct sockaddr *) &client_address, &client_addrlength)) < 0) {
#ifdef DEBUG_VERBOSE
        CONSOLE_LOG_WARN("%s", "accept failure");
#endif
        LOG_ERROR("%s:errno is:%d", "accept error", errno);
        return;
    }

    /* get address */
#ifdef DEBUG_VERBOSE
    char remoteIP[INET6_ADDRSTRLEN];
    char *remote_addr = (char *) inet_ntop(client_address.sin_family,
                                           get_in_addr((sockaddr *) &client_address), remoteIP,
                                           INET6_ADDRSTRLEN);
    CONSOLE_LOG_INFO("address: %s", remote_addr);
#endif

    /* if too much */
    if (HttpHandler::m_user_count == MAX_FD_){ // undealt HTTP >= 65532
        send(connfd, "Internal server busy\r\n", strlen("Internal server busy\r\n"), 0);
        close(connfd);
#ifdef DEBUG_VERBOSE
        CONSOLE_LOG_WARN("ERROR: Internal server busy");
#endif
        LOG_ERROR("ERROR: Internal server busy: %s", errno);
        return;
    }


    /* add to time list */
    Timer *timer = new Timer(connfd, client_address, &users_[connfd], time(NULL)+3*TIMESLOT);
    timer_lst_.add_timer(timer); // add TIMER to TIMERLIST

    /* add to thread pool */
    users_[connfd].init(connfd, client_address, timer); // init HTTPCON
};

