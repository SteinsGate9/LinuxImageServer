#include <map>
#include <mysql/mysql.h>

#include "http_conn.h"
#include "log.h"
#include "define.h"
#include "common.h"


/********************************************
 * global chars
********************************************/
const char *ok_200_title = "OK";
const char *error_400_title = "Bad Request";
const char *error_400_form = "Your request has bad syntax or is inherently impossible to staisfy.\n";
const char *error_403_title = "Forbidden";
const char *error_403_form = "You do not have permission to get file form this server.\n";
const char *error_404_title = "Not Found";
const char *error_404_form = "The requested file was not found on this server.\n";
const char *error_500_title = "Internal Error";
const char *error_500_form = "There was an unusual problem serving the request file.\n";

#ifdef DEBUG
const char correct[1000] = "GET / HTTP/1.1\r\nUser-Agent: 441UserAgent/1.0.0\r\n\r\n";
char correct2[1000] = "\0";
#endif

/********************************************
 * static var
********************************************/
int HttpHandler::m_epollfd = -1;
int HttpHandler::m_user_count = 0;
map<string, string> HttpHandler::users;



/********************************************
 * public
********************************************/
void HttpHandler::init(int sockfd, const sockaddr_in &addr, Timer* timer){
    /* sockfd */
    add_fd(m_epollfd, sockfd, true, false);
    m_sockfd = sockfd;

    /* count */
    m_user_count++;

    /* other */
    m_address = addr;
    m_timer = timer;
    init_();
}

void HttpHandler::close(bool real_close){
    if (real_close) {
        /* sockfd */
        remove_fd(m_epollfd, m_sockfd);
        m_sockfd = -1; /* lazy delete */

        /* count */
        m_user_count--;
    }
}



#ifdef DEBUG
void HttpHandler::process() {
    /* read from connfd */
    read_();

    /* parse the request */
    HTTP_STATE ret = parse_();

    /* write to */
    write_(ret);


#ifdef DEBUG_VERBOSE
//    CONSOLE_LOG_INFO("%ld get: %ld", this_thread::get_id(), strlen(m_read_buf));
//    CONSOLE_LOG_INFO("get messsage: %s with size %ld", read_buf, strlen(correct));
//
//    if (strcasecmp(correct2, m_read_buf) != 0 ) {
//        strcpy(m_read_buf, "HTTP/1.1 400 Bad Request\r\n");
//    }
//    else{
//    }
//    send(m_sockfd, m_read_buf, strlen(m_read_buf), 0);
//
//    CONSOLE_LOG_INFO("%ld send: %ld", this_thread::get_id(), strlen(m_read_buf));
#endif
}
#endif
#ifndef DEBUG
void HttpHandler::process()
{
    HTTP_CODE read_ret = process_read();
    if (read_ret == NO_REQUEST)
    {
        mod_fd(m_epollfd, m_sockfd, EPOLLIN);
        return;
    }
    bool write_ret = process_write(read_ret);
    if (!write_ret)
    {
        close();
    }
    mod_fd(m_epollfd, m_sockfd, EPOLLOUT);
}
#endif

#ifdef SYNSQL
void HttpHandler::initmysql_result(){
    //先从连接池中取一个连接
    auto connPool = connectionPool::get_instance("localhost", "root", "123456", "yourdb", 3306, 8);

    MYSQL *mysql = connPool->get_connection();

    //在user表中检索username，passwd数据，浏览器端输入
    if (mysql_query(mysql, "SELECT username,passwd FROM user")){
        LOG_ERROR("SELECT error:%s\n", mysql_error(mysql));
        CONSOLE_LOG_ERROR("SELECT error:%s\n", mysql_error(mysql));
    }

    //从表中检索完整的结果集
    MYSQL_RES *result = mysql_store_result(mysql);

    //返回结果集中的列数
//    int num_fields = mysql_num_fields(result);

    //返回所有字段结构的数组
//    MYSQL_FIELD *fields = mysql_fetch_fields(result);

    //从结果集中获取下一行，将对应的用户名和密码，存入map中
    while (MYSQL_ROW row = mysql_fetch_row(result)) {
        string temp1(row[0]);
        string temp2(row[1]);
        users[temp1] = temp2;
    }
    //将连接归还连接池
    connPool->release_connection(mysql);
}
#endif
#ifdef CGISQLPOOL
void HttpHandler::initresultFile(connectionPool *connPool)
{
    ofstream out("./CGImysql/id_passwd.txt");
    //先从连接池中取一个连接
    MYSQL *mysql = connPool->get_connection();

    //在user表中检索username，passwd数据，浏览器端输入
    if (mysql_query(mysql, "SELECT username,passwd FROM user"))
    {
        LOG_ERROR("SELECT error:%s\n", mysql_error(mysql));
    }

    //从表中检索完整的结果集
    MYSQL_RES *result = mysql_store_result(mysql);

    //返回结果集中的列数
    int num_fields = mysql_num_fields(result);

    //返回所有字段结构的数组
    MYSQL_FIELD *fields = mysql_fetch_fields(result);

    //从结果集中获取下一行，将对应的用户名和密码，存入map中
    while (MYSQL_ROW row = mysql_fetch_row(result))
    {
        string temp1(row[0]);
        string temp2(row[1]);
        out << temp1 << " " << temp2 << endl;
        users[temp1] = temp2;
    }
    //将连接归还连接池
    connPool->release_connection(mysql);
    out.close();
}
#endif

void HttpHandler::init_()
{
    mysql = NULL;
    bytes_to_send = 0;
    bytes_have_send = 0;
    m_check_state = CHECK_STATE_REQUESTLINE;
    m_linger = true;
    m_method = GET;
    m_url = 0;
    m_version = 0;
    m_content_length = 0;
    m_host = 0;
    m_start_line = 0;
    m_checked_idx = 0;
    m_read_idx = 0;
    m_write_idx = 0;
    m_cgi = 0;
    memset(m_read_buf, '\0', READ_BUFFER_SIZE);
    memset(m_write_buf, '\0', WRITE_BUFFER_SIZE);
    memset(m_real_file, '\0', FILENAME_LEN);
}


/********************************************
 * parse
********************************************/
void HttpHandler::read_() {
    if (m_read_idx >= READ_BUFFER_SIZE) {
        LOG_ERROR("read buffer over flow address: %s", inet_ntoa(m_address.sin_addr));
        close();
        return;
    }
    int bytes_read = 0;
    int tried_times = 10;
    while (true) {
        bytes_read = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);
        if (bytes_read == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK){  /* no data available */
                if (--tried_times == 0)
                    break;
            }
            else {/* other error */
                LOG_ERROR("read request failed address: %s", inet_ntoa(m_address.sin_addr));
                close();
                return;
            }
        }
        else if (bytes_read == 0) { /* already shut down */
            LOG_ERROR("connection shut down address: %s", inet_ntoa(m_address.sin_addr));
            close();
            return;
        }
        m_read_idx += bytes_read;
    }
    return;
}


HttpHandler::HTTP_STATE HttpHandler::parse_(){

    LINE_STATE line_status;
    HTTP_STATE ret;
    char *text = nullptr;

    while ((m_check_state == CHECK_STATE_CONTENT && line_status == LINE_OK) || ((line_status = parse_line_()) == LINE_OK)){
        text = get_line();
        m_start_line = m_checked_idx;

        switch (m_check_state) {
            /* possible outputs =>
             * 1) parse line failed => ERROR_FORMAT
             * 2) parse line ok & parse request failed => ERROR_FORMAT
             * 3) parse link ok & parse request ok => PARSING & CHECK_HEADER */
            case CHECK_STATE_REQUESTLINE:{
                ret = parse_request_(text);
                if (ret == ERROR_FORMAT)
                    return ERROR_FORMAT;
                break;
            }
            /* possible outputs =>
             * 1) parse line failed => ERROR_FORMAT
             * 2) parse line ok & parse request failed => ERROR_FORMAT
             * 3) parse line ok & parse request ok => looping
             * 4) parse link ok & parse request finished => do request
             * 5) parse link ok & parse request finished & still have contents => PARSING & CHECK_CONTENT */
            case CHECK_STATE_HEADER:{  /* loop n times */
                ret = parse_headers_(text);
                if (ret == ERROR_FORMAT)
                    return ERROR_FORMAT;
                else if (ret == READY_REQUEST){
                    return do_request_();
                }
                break;
            }
            /* possible outputs:
             * 1) parse line failed => ERROR_FORMAT
             * 2) parse link ok & content full => do request */
            case CHECK_STATE_CONTENT:{
                ret = parse_content_(text);
                if (ret == ERROR_FORMAT)
                    return ERROR_FORMAT;
                if (ret == READY_REQUEST){
                    return do_request_();
                }
                break;
            }
            default:
                return INTERNAL_ERROR;
        }
    }

    return ERROR_FORMAT;
}

HttpHandler::LINE_STATE HttpHandler::parse_line_(){
    char *str = m_read_buf + m_checked_idx;

    auto slashr = strpbrk(str, "\r");
    auto slashn = strpbrk(str, "\n");

    auto slashr_ok = slashr && (int)(slashr - str) <= m_read_idx -1;
    auto slashn_ok = slashn && (int)(slashn - str) <= m_read_idx -1;
    if (slashn_ok && slashr_ok){
        int slashn_index = (int) (slashn - str);
        int slashr_index = (int) (slashr - str);
        m_checked_idx += slashn_index + 1;

        if (slashr_index == slashn_index-1) {
            str[slashr_index] = '\0';
            str[slashn_index] = '\0';
            return HttpHandler::LINE_OK;
        }
        else {
            return HttpHandler::LINE_BAD;
        }
    }
    else if (slashn_ok){
        int slashn_index = (int) (slashn - str);
        m_checked_idx += slashn_index + 1;
        return HttpHandler::LINE_BAD;
    }
    else if (slashr_ok){
        int slashr_index = (int) (slashr - str);
        m_checked_idx += slashr_index + 1;
        return slashr_index==m_read_idx-1?HttpHandler::LINE_OPEN:HttpHandler::LINE_BAD;
    }
    else {
        m_checked_idx = m_read_idx;
        return HttpHandler::LINE_OPEN;
    }
}

HttpHandler::HTTP_STATE HttpHandler::parse_request_(char *text){
    /* go locate m_url */
    m_url = strpbrk(text, " ");
    if (!m_url){
        return ERROR_FORMAT;
    }
    *m_url++ = '\0';
    assert(m_url);

    /* get method */
    char *method = text;
    if (strcasecmp(method, "GET") == 0)
        m_method = GET;
    else if (strcasecmp(method, "POST") == 0){
        m_method = POST;
        m_cgi = 1;
    }
    else if (strcasecmp(method, "HEAD") == 0){
        m_method = HEAD;
    }
    else
        return ERROR_FORMAT;

    /* go locate version */
    m_version = strpbrk(m_url, " ");
    if (!m_version) {
        return ERROR_FORMAT;
    }
    *m_version++ = '\0';
    assert(m_version);

    /* get url */
    if (strncasecmp(m_url, "http://", 7) == 0){
        m_url += 7;
        m_url = strchr(m_url, '/');
    }
    else if (strncasecmp(m_url, "https://", 8) == 0){
        m_url += 8;
        m_url = strchr(m_url, '/');
    }
    if (!m_url || *m_url != '/')
        return ERROR_FORMAT;

    /* get version */
    if (strcasecmp(m_version, "HTTP/1.1") != 0)
        return ERROR_FORMAT;

    /* when m_url == "/" */
    if (strlen(m_url) == 1)
        strcat(m_url, "judge.html");

    /* go for next */
    m_check_state = CHECK_STATE_HEADER;
    return PARSERING;
}

HttpHandler::HTTP_STATE HttpHandler::parse_headers_(char *text){
    if (text[0] == '\0'){
        if (m_content_length != 0){
            m_check_state = CHECK_STATE_CONTENT;
            return PARSERING;
        }
        return READY_REQUEST;
    }
    else if (strncasecmp(text, "Connection:", 11) == 0){
        text += 11;
        text += strspn(text, " \t");
        if (strncasecmp(text, "close", 5) == 0){
            m_linger = false;
        }
    }
    else if (strncasecmp(text, "Content-length:", 15) == 0){
        text += 15;
        text += strspn(text, " \t");
        m_content_length = strtol(text, nullptr, 10);
        if (errno == EINVAL){
            return ERROR_FORMAT;
        }
    }
    else if (strncasecmp(text, "Host:", 5) == 0){
        text += 5;
        text += strspn(text, " \t");
        m_host = text;
    }
    else if (strncasecmp(text, "User-Agent:", 11) == 0){
        text += 11;
        text += strspn(text, " \t");
        m_useragent = text;
    }
    else if (strncasecmp(text, "Origin:", 7) == 0){
        text += 7;
        text += strspn(text, " \t");
        m_origin = text;
    }
    else if (strncasecmp(text, "Content-Type:", 13) == 0){
        text += 13;
        text += strspn(text, " \t");
        m_content_type = text;
    }
    else if (strncasecmp(text, "Accept-Encoding:", 16) == 0){
        text += 16;
        text += strspn(text, " \t");
        m_encoding = text;
    }
    else if (strncasecmp(text, "Upgrade-Insecure-Requests:", 26) == 0){
        text += 26;
        text += strspn(text, " \t");
        m_upgrade = text;
    }
    else if (strncasecmp(text, "Accept:", 7) == 0){
        text += 7;
        text += strspn(text, " \t");
        m_accept = text;
    }
    else if (strncasecmp(text, "Referer:", 8) == 0){
        text += 8;
        text += strspn(text, " \t");
        m_referer = text;
    }
    else if (strncasecmp(text, "Accept_Language:", 16) == 0){
        text += 16;
        text += strspn(text, " \t");
        m_language = text;
    }
    else {
        LOG_INFO("oop!unknow header: %s", text);
    }
    text = strpbrk(text, " \t");
    if (text)
        *text = '\0';

    return PARSERING;
}

HttpHandler::HTTP_STATE HttpHandler::parse_content_(char *text){ /* request line \0 crlf content crlf ... */
    if (text - m_read_buf + m_content_length <= m_read_idx){
        text[m_content_length] = '\0';
        m_content = text;
        return READY_REQUEST;
    }
    else{
        return ERROR_FORMAT;
    }

}

HttpHandler::HTTP_STATE HttpHandler::do_request_(){
    strcpy(m_real_file, __ROOTPATH__);
    int len = strlen(__ROOTPATH__);
    const char *p = strrchr(m_url, '/');

    /* deal with CGI */
    if (m_cgi == 1 && (*(p + 1) == '2' || *(p + 1) == '3')) {
        /* get real file name & path */
        char *m_url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(m_url_real, "/");
        strcat(m_url_real, m_url + 2); /* url */
        strcat(m_real_file, m_url_real);
        free(m_url_real);

        /* get id &*/
        char name[100], password[100];
        int i;
        for (i = 5; m_content[i] != '&'; ++i) /* user=adsda&password=tibrak-1nufXu-cubkid */
            name[i - 5] = m_content[i];
        name[i - 5] = '\0';

        int j;
        for (i = i+10, j = 0; m_content[i] != '\0'; ++i, ++j)
            password[j] = m_content[i];
        password[j] = '\0';

//同步线程登录校验
#ifdef SYNSQL
        std::mutex mutex_;
        /* if register */
        if (*(p + 1) == '3') {
            /* insert for user name */
            char *sql_insert = (char *)malloc(sizeof(char) * 200);
            strcpy(sql_insert, "INSERT INTO user(username, passwd) VALUES(");
            strcat(sql_insert, "'");
            strcat(sql_insert, name);
            strcat(sql_insert, "', '");
            strcat(sql_insert, password);
            strcat(sql_insert, "')");

            mutex_.lock();
            /* if not found */
            if (users.find(name) == users.end()){
                int res = mysql_query(mysql, sql_insert);
                users.insert(pair<string, string>(name, password));
                if (!res)
                    strcpy(m_url, "/log.html");
                else
                    strcpy(m_url, "/registerError.html");
            }
            /* if found */
            else
                strcpy(m_url, "/registerError.html");
            mutex_.unlock();
        }
        /* if login */
        else if (*(p + 1) == '2'){
            if (users.find(name) != users.end() && users[name] == password)
                strcpy(m_url, "/welcome.html");
            else
                strcpy(m_url, "/logError.html");
        }
#endif


//CGI用连接池,注册在父进程,登录在子进程
#ifdef CGISQLPOOL
        //注册
        pthread_mutex_t lock;
        pthread_mutex_init(&lock, NULL);

        if (*(p + 1) == '3')
        {
            //如果是注册，先检测数据库中是否有重名的
            //没有重名的，进行增加数据
            char *sql_insert = (char *)malloc(sizeof(char) * 200);
            strcpy(sql_insert, "INSERT INTO user(username, passwd) VALUES(");
            strcat(sql_insert, "'");
            strcat(sql_insert, name);
            strcat(sql_insert, "', '");
            strcat(sql_insert, password);
            strcat(sql_insert, "')");

            if (users.find(name) == users.end())
            {
                pthread_mutex_lock(&lock);
                int res = mysql_query(mysql, sql_insert);
                users.insert(pair<string, string>(name, password));
                pthread_mutex_unlock(&lock);

                if (!res)
                {
                    strcpy(m_url, "/log.html");
                    pthread_mutex_lock(&lock);
                    //每次都需要重新更新id_passwd.txt
                    ofstream out("./CGImysql/id_passwd.txt", ios::app);
                    out << name << " " << password << endl;
                    out.close();
                    pthread_mutex_unlock(&lock);
                }
                else
                    strcpy(m_url, "/registerError.html");
            }
            else
                strcpy(m_url, "/registerError.html");
        }
        //登录
        else if (*(p + 1) == '2')
        {
            pid_t pid;
            int pipefd[2];
            if (pipe(pipefd) < 0)
            {
                LOG_ERROR("pipe() error:%d", 4);
                return BAD_REQUEST;
            }
            if ((pid = fork()) < 0)
            {
                LOG_ERROR("fork() error:%d", 3);
                return BAD_REQUEST;
            }

            if (pid == 0)
            {
                //标准输出，文件描述符是1，然后将输出重定向到管道写端
                dup2(pipefd[1], 1);
                //关闭管道的读端
                close(pipefd[0]);
                //父进程去执行cgi程序，m_real_file,name,password为输入
                execl(m_real_file, name, password, "./CGImysql/id_passwd.txt", NULL);
            }
            else
            {
                //子进程关闭写端，打开读端，读取父进程的输出
                close(pipefd[1]);
                char result;
                int ret = read(pipefd[0], &result, 1);

                if (ret != 1)
                {
                    LOG_ERROR("管道read error:ret=%d", ret);
                    return BAD_REQUEST;
                }

                LOG_INFO("%s", "登录检测");
                Log::get_instance()->flush();
                //当用户名和密码正确，则显示welcome界面，否则显示错误界面
                if (result == '1')
                    strcpy(m_url, "/welcome.html");
                else
                    strcpy(m_url, "/logError.html");

                //回收进程资源
                waitpid(pid, NULL, 0);
            }
        }

#endif

//CGI多进程登录校验,不用数据库连接池
//子进程完成注册和登录
#ifdef CGISQL
        //fd[0]:读管道，fd[1]:写管道
        pid_t pid;
        int pipefd[2];
        if (pipe(pipefd) < 0)
        {
            LOG_ERROR("pipe() error:%d", 4);
            return BAD_REQUEST;
        }
        if ((pid = fork()) < 0)
        {
            LOG_ERROR("fork() error:%d", 3);
            return BAD_REQUEST;
        }

        if (pid == 0)
        {
            //标准输出，文件描述符是1，然后将输出重定向到管道写端
            dup2(pipefd[1],1);
            //关闭管道的读端
            close(pipefd[0]);
            //父进程去执行cgi程序，m_real_file,name,password为输入
            //./check.cgi name password
            execl(m_real_file, &flag, name,password,NULL);
        }
        else
        {
            //子进程关闭写端，打开读端，读取父进程的输出
            close(pipefd[1]);
            char result;
            int ret = read(pipefd[0], &result, 1);

            if (ret != 1)
            {
                LOG_ERROR("管道read error:ret=%d", ret);
                return BAD_REQUEST;
            }
            if (flag == '2')
            {
                //printf("登录检测\n");
                LOG_INFO("%s", "登录检测");
                Log::get_instance()->flush();
                //当用户名和密码正确，则显示welcome界面，否则显示错误界面
                if (result == '1')
                    strcpy(m_url, "/welcome.html");
                else
                    strcpy(m_url, "/logError.html");
            }
            else if (flag == '3')
            {
                LOG_INFO("%s", "注册检测");
                Log::get_instance()->flush();
                //当成功注册后，则显示登陆界面，否则显示错误界面
                if (result == '1')
                    strcpy(m_url, "/log.html");
                else
                    strcpy(m_url, "/registerError.html");
            }
            //回收进程资源
            waitpid(pid, NULL, 0);
        }
#endif
    }

    if (*(p + 1) == '0') { /* POST /0 new user */
        char *m_url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(m_url_real, "/register.html");
        strncpy(m_real_file + len, m_url_real, strlen(m_url_real));

        free(m_url_real);
    }
    else if (*(p + 1) == '1') { /* POST /1 log in */
        char *m_url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(m_url_real, "/log.html");
        strncpy(m_real_file + len, m_url_real, strlen(m_url_real));

        free(m_url_real);
    }
    else if (*(p + 1) == '5') { /* POST /5 get picture */
        char *m_url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(m_url_real, "/picture.html");
        strncpy(m_real_file + len, m_url_real, strlen(m_url_real));

        free(m_url_real);
    }
    else if (*(p + 1) == '6')
    {
        char *m_url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(m_url_real, "/video.html");
        strncpy(m_real_file + len, m_url_real, strlen(m_url_real));

        free(m_url_real);
    }
    else if (*(p + 1) == '7')
    {
        char *m_url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(m_url_real, "/fans.html");
        strncpy(m_real_file + len, m_url_real, strlen(m_url_real));

        free(m_url_real);
    }
    else
        strncpy(m_real_file + len, m_url, FILENAME_LEN - len - 1);

    if (stat(m_real_file, &m_file_stat) < 0) { /* if no file */
        return ERROR_FILENOTFOUND;
    }
    if (!(m_file_stat.st_mode & S_IROTH)) {
        return ERROR_ACCESSFAILED;
    }
    if (S_ISDIR(m_file_stat.st_mode)) {
        return ERROR_FORMAT;
    }
    int fd;
    if ((fd = open(m_real_file, O_RDONLY)) == -1) {
        return ERROR_ACCESSFAILED;
    }
    if ((m_file_address = (char *)mmap(0, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0)) == (void *)(-1)){
        return INTERNAL_ERROR;
    }
    if (::close(fd) == -1){
        return INTERNAL_ERROR;
    }
    return FINISHED_REQUEST;
}


/********************************************
 * write related
********************************************/
void HttpHandler::write_(HttpHandler::HTTP_STATE ret){
    if (!write_to_buffer_(ret)){
        LOG_ERROR("buffer over flow for address %s", inet_ntoa(m_address.sin_addr));
        close();
        return;
    }
    write_to_fd_();
}

bool HttpHandler::write_to_buffer_(HttpHandler::HTTP_STATE ret){ /* TODO: possible multithread issue*/
    switch (ret){
        case INTERNAL_ERROR:{
            add_status_(500, error_500_title);
            add_headers_(strlen(error_500_form));
            if (!add_content_(error_500_form))
                return false;
            break;
        }
        case ERROR_FORMAT:{
            add_status_(400, error_400_title);
            add_headers_(strlen(error_400_form));
            if (!add_content_(error_400_form))
                return false;
            break;
        }
        case ERROR_ACCESSFAILED:{
            add_status_(403, error_403_title);
            add_headers_(strlen(error_403_form));
            if (!add_content_(error_403_form))
                return false;
            break;
        }
        case ERROR_FILENOTFOUND:{
            add_status_(403, error_403_title);
            add_headers_(strlen(error_403_form));
            if (!add_content_(error_403_form))
                return false;
            break;
        }
        case FINISHED_REQUEST:{
            add_status_(200, ok_200_title);
            if (m_file_stat.st_size != 0){ /* have file to send */
                add_headers_(m_file_stat.st_size);
                m_iv[1].iov_base = m_file_address;
                m_iv[1].iov_len = m_file_stat.st_size;
                m_iv_count += 1;
                bytes_to_send = m_file_stat.st_size;
            }
            else{
                const char *ok_string = "<html><body></body></html>";
                add_headers_(strlen(ok_string));
                if (!add_content_(ok_string))
                    return false;
            }
            break;
        }
        default:
            return false;
    }
    m_iv[0].iov_base = m_write_buf;
    m_iv[0].iov_len = m_write_idx;
    m_iv_count += 1;
    bytes_to_send += m_write_idx;
    return true;
}

bool HttpHandler::add_response_(const char *format, ...){
    if (m_write_idx >= WRITE_BUFFER_SIZE)
        return false;
    va_list arg_list;
    va_start(arg_list, format);
    int len = vsnprintf(m_write_buf + m_write_idx, WRITE_BUFFER_SIZE - 1 - m_write_idx, format, arg_list);
    if (len >= (WRITE_BUFFER_SIZE - 1 - m_write_idx)){
        va_end(arg_list);
        return false;
    }
    m_write_idx += len;
    va_end(arg_list);
    return true;
}

bool HttpHandler::add_status_(int status, const char *title){
    return add_response_("%s %d %s\r\n", "HTTP/1.1", status, title);
}


bool HttpHandler::add_headers_(int content_len){
    add_content_length_(content_len);
    add_linger_();
    add_clrf_();
    return true;
}

bool HttpHandler::add_content_length_(int content_len)
{
    return add_response_("Content-Length:%d\r\n", content_len);
}
bool HttpHandler::add_linger_()
{
    return add_response_("Connection:%s\r\n", (m_linger) ? "keep-alive" : "close");
}
bool HttpHandler::add_clrf_()
{
    return add_response_("%s", "\r\n");
}

bool HttpHandler::add_content_(const char *content)
{
    return add_response_("%s", content);
}

bool HttpHandler::add_content_type_()
{
    return add_response_("Content-Type:%s\r\n", "text/html");
}

bool HttpHandler::write_to_fd_(){
    int temp = 0;
    int newadd = 0;

    while (true){
        /* write back */
        temp = writev(m_sockfd, m_iv, m_iv_count);
        if (temp > 0){ /* if succeeded*/
            /* updates */
            bytes_have_send += temp;
            newadd = bytes_have_send - m_write_idx;
            bytes_to_send -= temp;

            /* if finished */
            if (bytes_to_send <= 0){
                unmap();
                if (m_linger){
                    init_();
                    LOG_INFO("sent all data:%d and linger", bytes_have_send);
                    return true;
                }
                else {
                    close();
                    LOG_INFO("sent all data:%d and closed %s", bytes_have_send, inet_ntoa(m_address.sin_addr));
                    return false;
                }
            }
        }
        if (temp <= -1){ /* if error */
            if (errno == EAGAIN){ /* non-blocking update info */
                if (bytes_have_send >= (int)m_iv[0].iov_len){
                    m_iv[0].iov_len = 0;
                    m_iv[1].iov_base = m_file_address + newadd;
                    m_iv[1].iov_len = bytes_to_send;
                }
                else{
                    m_iv[0].iov_base = m_write_buf + bytes_to_send;
                    m_iv[0].iov_len = m_iv[0].iov_len - bytes_have_send;
                }
            }
            else { /* other error */
                close();
                LOG_INFO("error [%s] occurred for address %s", strerror(errno), inet_ntoa(m_address.sin_addr));
                return false;
            }
        }
    }


}

