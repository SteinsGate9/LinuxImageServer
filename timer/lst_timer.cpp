//
// Created by GORGEOUS MAN on 2020-04-15.
//
#include <time.h>
#include "lst_timer.h"
#include "../http/http_conn.h"

util_timer::util_timer(int sockfd, sockaddr_in address, http_conn* data, time_t expire):
    data(data),expire(expire),prev( NULL ), next( NULL ){}

util_timer::~util_timer(){

}

// set expire time
void util_timer::set_expire(time_t expire_) {expire = expire_;}

// out of time call HTTP_CONN var to close connections
void util_timer::cb_func(){
    epoll_ctl(data->m_epollfd, EPOLL_CTL_DEL, data->m_sockfd, 0);
    close(data->m_sockfd);
    LOG_INFO("closing fd %d", data->m_sockfd);
    Log::get_instance()->flush();
}

// delete all util_timers
sort_timer_lst::~sort_timer_lst() {
    util_timer* tmp = head;
    while(tmp){
        head = tmp->next;
        delete tmp;
        tmp = head;
    }
}

// add to linked list
void sort_timer_lst::add_timer(util_timer* timer){
    if(!timer) return;
    if(!head){ // check head
        head = tail = timer;
        return;
    }
    if(timer->expire < head->expire){ // check head
        timer->next = head;
        head->prev = timer;
        head = timer;
        return;
    }
    _add_timer(timer, head);
}

void sort_timer_lst::adjust_timer( util_timer* timer ){ // adjust timer to correct place
    if( !timer ) return;

    util_timer* tmp = timer->next;
    if(!tmp || (timer->expire < tmp->expire)) return; // if tail or no need to adjust

    if(timer == head){ // if head
        head = head->next;
        head->prev = NULL;
        timer->next = NULL;
        _add_timer( timer, head );
    }
    else{ // else
        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;
        _add_timer( timer, timer->next );
    }
}

void sort_timer_lst::del_timer( util_timer* timer ){
    if(!timer) return;

    if((timer == head) && (timer == tail)){ // if size == 1
        head = NULL; tail = NULL;
        delete timer;
        return;
    }
    if(timer == head){ // if size != 1 && head
        head = head->next; head->prev = NULL;
        delete timer;
        return;
    }
    else if(timer == tail){ // if size != 1 && tail
        tail = tail->prev; tail->next = NULL;
        delete timer;
        return;
    }
    else {
        timer->prev->next = timer->next; // if in the middle
        timer->next->prev = timer->prev;
    }
    delete timer;
}

void sort_timer_lst::timeout(){ // IMPORTANT: time out happens call timeout(), del expired timers
    if(!head) return;

    LOG_INFO("%s","timer timeout"); // log & flush
    Log::get_instance()->flush();

    time_t cur = time(NULL);
    util_timer* tmp = head;
    while(tmp){
        if(cur < tmp->expire) break;
        tmp->cb_func();
        head = tmp->next;
        if(head) head->prev = NULL;
        delete tmp;
        tmp = head;
    }
}

void sort_timer_lst::_add_timer( util_timer* timer, util_timer* lst_head ) { // add timer after head
    util_timer *prev = lst_head;
    util_timer *tmp = prev->next;
    while (tmp) {
        if (timer->expire < tmp->expire) {
            prev->next = timer;
            timer->next = tmp;
            tmp->prev = timer;
            timer->prev = prev;
            break;
        }
        prev = tmp;
        tmp = tmp->next;
    }
    if (!tmp) {
        prev->next = timer;
        timer->prev = prev;
        timer->next = NULL;
        tail = timer;
    }
}