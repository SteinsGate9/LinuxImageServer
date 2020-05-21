#ifndef HttpHandlerECTION_H
#define HttpHandlerECTION_H
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <map>
#include <openssl/ssl.h>

#include "locker.h"
#include "sql_connection_pool.h"
#include "lst_timer.h"

/********************************************
 * extern
********************************************/
extern const char correct[1000];
extern char correct2[1000];


/********************************************
 * define
********************************************/
#define TIMESLOT_ 5
enum class ClientState {
    INVALID,
    READY_FOR_READ,
    READY_FOR_WRITE,
    WAITING_FOR_CGI,
    CGI_FOR_READ,
    CGI_FOR_WRITE
};

enum class CLIENT_TYPE {
    INVALID_CLIENT,
    HTTP_CLIENT,
    HTTPS_CLIENT
};


/********************************************
 * class
********************************************/
class HttpHandler
{
    friend Timer;
public:
    static const int FILENAME_LEN = 200;
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;
    enum METHOD /* HTTP methods */
    {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };
    enum PARSE_STATE /* a FSM for parsing */
    {
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };
    enum HTTP_STATE /* HTTP state */
    {
        PARSERING,
        ERROR_FORMAT,
        READY_REQUEST,

        ERROR_ACCESSFAILED,
        ERROR_FILENOTFOUND,
        FINISHED_REQUEST,

        CLOSED_CONNECTION,

        INTERNAL_ERROR

    };
    enum LINE_STATE /* parsing a line */
    {
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };

public:
    HttpHandler() {}
    ~HttpHandler() {}

public:
    /* socket related */
    void init(int sockfd, const sockaddr_in &addr, Timer* timer, CLIENT_TYPE, SSL*); /* init with fd & stuff */
    void close(bool real_close = true); /* deal with close */
    void process(); /* worker function */

    /* init related */
    static void initmysql_result();
    static void initresultFile();

    /* helper functions */
    sockaddr_in *get_address(){
        return &m_address;
    }
    Timer* adjust_timer(){
        m_timer->expire = (time(NULL) + 3*TIMESLOT_); //若有数据传输，则将定时器往后延迟3个单位
        return m_timer;
    }

    /* for test purpose */
#ifdef DEBUG
    void set_buf(char* str){
        strcpy(m_read_buf, str);
        m_read_idx = 123;
        m_checked_idx = 0;
    }
    void set_readidx(int i){ m_read_idx = i;}
    char* get_buf(){
        return m_read_buf;
    }
    int get_check_idx(){
        return m_checked_idx;
    }
    HTTP_STATE parse(){ return parse_(); }
    LINE_STATE parse_line(){
        return parse_line_();
    }
    HTTP_STATE parse_request(char *text) {return parse_request_(text);}
    HTTP_STATE parse_headers(char *text) {return parse_headers_(text);}
    HTTP_STATE parse_content(char *text) {return parse_content_(text);}
    HTTP_STATE do_request() {return do_request_();};
    char* get_my_url(){return m_url;};
    METHOD get_my_method(){return m_method;};
    char* get_my_version(){return m_version;};
    char* get_my_host(){return m_host;};
    int get_my_length(){return m_content_length;};
    bool get_my_linger(){return m_linger;};
    char *get_my_useragent(){return m_useragent;};
#endif


private:
    /* init related */
    void init_();/* put same stuff in private */

    /* read related */
    void read_(); /* read HTTP message function needed */
    HTTP_STATE parse_();
    LINE_STATE parse_line_(); /* a line is header ows : ows value crlf or request token sp text sp text t_crlf or context */
    HTTP_STATE parse_request_(char *text);
    HTTP_STATE parse_headers_(char *text);
    HTTP_STATE parse_content_(char *text);
    HTTP_STATE do_request_();

    /* write related */
    void write_(HTTP_STATE); /* write HTTP respond function needed */
    bool write_to_buffer_(HTTP_STATE);
    void write_to_fd_();
    bool add_response_(const char *format, ...);
    /* status */
    bool add_status_(int status, const char *title); /* HTTP/1.1 500 InternalError/... crlf */
    /* headers */
    bool add_headers_(int content_length);
    bool add_content_length_(int content_length);
    bool add_linger_();
    bool add_clrf_();
    /* content */
    bool add_content_(const char *content);
    bool add_content_type_();

    /* helper functions */
    char *get_line() { return m_read_buf + m_start_line; };
    void unmap(){
        if (m_file_address){
            munmap(m_file_address, m_file_stat.st_size);
            m_file_address = 0;
        }
    }


public:
    /* for all */
    static int m_epollfd; /* this is unchanged */
    static int m_user_count; /* this need to be maintained */
    static map<string, string> users; /* this is unchanged */

    /* from outside*/
    Timer* m_timer; /* to support timer */
    MYSQL *mysql;  /* to support sql */


private:
    /* from outside */
    int m_sockfd;
    sockaddr_in m_address;
    CLIENT_TYPE m_type;
    SSL* m_sslcontent;

    /* read related */
    char m_read_buf[READ_BUFFER_SIZE];
    int m_read_idx; /* next byte offset to read */
    /* parser related */
    PARSE_STATE m_check_state;
    int m_checked_idx; /* next bype to check */
    int m_start_line; /* start line for FSM */
    /* parse request */
    METHOD m_method;
    int m_cgi;       /* set if POST */
    char *m_url;
    char *m_version;
    /* parse header */
    bool m_linger;
    int m_content_length;
    char *m_host;
    char *m_useragent;
    char *m_origin;
    char *m_content_type;
    char *m_encoding;
    char *m_upgrade;
    char *m_accept;
    char *m_referer;
    char *m_language;
    /* parse request */
    char *m_content; /* store content */
    /* do request */
    char m_real_file[FILENAME_LEN]; /* file to send*/
    struct stat m_file_stat;
    char *m_file_address;

    /* write related */
    char m_write_buf[WRITE_BUFFER_SIZE];
    int m_write_idx; /* next bype offset to write */
    /* io related */
    struct iovec m_iv[2];
    int m_iv_count;
    int bytes_to_send;
    int bytes_have_send;
};

#endif
