#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include <pthread.h>
#include <thread>

#include "log.h"


    Log::Log():m_count(0), m_is_async(false) {
    }

    Log::~Log() {
        LOG_INFO("%s", "closing log system \n");
        if (m_fp != nullptr) {
            fclose(m_fp);
        }
        delete[] m_buf;
    }

    bool Log::init(const char *file_name, int log_buf_size, int split_lines, int max_queue_size) {
        /* if max_queue_size >= 1, then async*/
        if (max_queue_size >= 1) {
            m_is_async = true;
            m_log_queue = new BlockQueue<string>(max_queue_size);
            m_thread = std::thread(flush_log_thread);
        }

        /* buffer init */
        m_log_buf_size = log_buf_size;
        m_buf = new char[m_log_buf_size];
        memset(m_buf, '\0', m_log_buf_size);
        m_split_lines = split_lines;

        /* time related */
        time_t t = time(NULL);
        struct tm *sys_tm = localtime(&t);
        struct tm my_tm = *sys_tm;

        /* find last '/' */
        const char *p = strrchr(file_name, '/');
        char log_full_name[256] = {0};
        //相当于自定义日志名
        //若输入的文件名没有/,则直接将时间+文件名作为日志名
        if (p == NULL) {
            snprintf(log_full_name, 255, "%d_%02d_%02d_%s", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday,
                     file_name);
        } else {
            //将/的位置向后移动一个位置，然后复制到logname中
            //p - file_name + 1是文件所在路径文件夹的长度
            //dirname相当于./
            strcpy(log_name, p + 1);
            strncpy(dir_name, file_name, p - file_name + 1);
            //后面的参数跟format有关
            snprintf(log_full_name, 255, "%s%d_%02d_%02d_%s", dir_name, my_tm.tm_year + 1900, my_tm.tm_mon + 1,
                     my_tm.tm_mday, log_name);
        }
        m_today = my_tm.tm_mday;

        if ((m_fp = fopen(log_full_name, "a")) == nullptr) {
            CONSOLE_LOG_ERROR_noexit("log init failed");
            return false;
        }
        return true;
    }

void Log::write_log(int level, const char *format, ...)
{
    struct timeval now = {0, 0};
    gettimeofday(&now, NULL);
    time_t t = now.tv_sec;
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;
    char s[16] = {0};
    switch (level)
    {
        case 0:
            strcpy(s, "[debug]:");
            break;
        case 1:
            strcpy(s, "[info]:");
            break;
        case 2:
            strcpy(s, "[warn]:");
            break;
        case 3:
            strcpy(s, "[erro]:");
            break;
        default:
            strcpy(s, "[info]:");
            break;
    }
    //写入一个log，对m_count++, m_split_lines最大行数
    pthread_mutex_lock(m_mutex);
    m_count++;
    //my_tm.tm_mday为每次写日志的时候判断当前时间,m_today是创建文件的时候记录的时间
    if (m_today != my_tm.tm_mday || m_count % m_split_lines == 0) //everyday log
    {
        //判断写入的日志行数超出了最大行数,若超过了最大行数,则新建文件
        //每次写日志时，需要判断当前日志，是不是今天的日志，如果不是今天的日志，需要将当前打开的关闭，再重新打开
        char new_log[256] = {0};
        fflush(m_fp);
        fclose(m_fp);
        char tail[16] = {0};
        //先把时间头写好
        snprintf(tail, 16, "%d_%02d_%02d_", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday);
        //如果是时间不是今天,则创建今天的日志,否则是超过了最大行,还是之前的日志名,只不过加了后缀, m_count/m_split_lines
        if (m_today != my_tm.tm_mday)
        {
            snprintf(new_log, 255, "%s%s%s", dir_name, tail, log_name);
            m_today = my_tm.tm_mday;
            m_count = 0;
        }
        else
        {
            snprintf(new_log, 255, "%s%s%s.%lld", dir_name, tail, log_name, m_count / m_split_lines);
        }
        m_fp = fopen(new_log, "a");
    }
    pthread_mutex_unlock(m_mutex);

    va_list valst;
    va_start(valst, format);

    string log_str;
    pthread_mutex_lock(m_mutex);

    //写入的具体时间内容格式
    //snprintf成功返回写字符的总数，其中不包括结尾的null字符
    int n = snprintf(m_buf, 48, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s ",
                     my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday,
                     my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec, now.tv_usec, s);
    //属于可变参数。用于向字符串中打印数据、数据格式用户自定义，返回写入到字符数组str中的字符个数(不包含终止符),最大不超过size
    int m = vsnprintf(m_buf + n, m_log_buf_size - 1, format, valst);
    m_buf[n + m] = '\n';
    m_buf[n + m + 1] = '\0';
    log_str = m_buf;
    pthread_mutex_unlock(m_mutex);

    //若m_is_async为true表示不同步，默认为同步
    //若异步,则将日志信息加入阻塞队列,同步则加锁向文件中写
    if (m_is_async && !m_log_queue->full())
    {
        m_log_queue->push(log_str);
    }
    else
    {
        pthread_mutex_lock(m_mutex);
        fputs(log_str.c_str(), m_fp);
        pthread_mutex_unlock(m_mutex);
    }

    va_end(valst);
}

void Log::flush(void)
{
    pthread_mutex_lock(m_mutex);
    //强制刷新写入流缓冲区
    fflush(m_fp);
    pthread_mutex_unlock(m_mutex);
}