/**
 * lru_replacer_test.cpp
 */
#include <iostream>
#include <cstdio>
#include <thread>
#include <map>
#include "gtest/gtest.h"

#include "http_conn.h"
#include "logger.h"
#include "client_pool.h"

namespace test_parse {
//    HttpHandler::LINE_STATE parse_line_(char* str){
//        char temp;
//        int m_checked_idx = 0;
//        int m_readindex = strlen(str);
//
//        auto slashr = strpbrk(str, "\r");
//        auto slashn = strpbrk(str, "\n");
//
//        auto slashr_ok = slashr && (int)(slashr - str) < m_readindex;
//        auto slashn_ok = slashn && (int)(slashn - str) < m_readindex;
//        if (slashn_ok && slashr_ok){
//            int slashn_index = (int) (slashn - str);
//            int slashr_index = (int) (slashr - str);
//            if (slashr_index == slashn_index-1) {
//                str[slashr_index] = '\0';
//                str[slashn_index] = '\0';
//                return HttpHandler::LINE_OK;
//            }
//            else {
//                return HttpHandler::LINE_BAD;
//            }
//        }
//        else if (slashn_ok){
//            return HttpHandler::LINE_BAD;
//        }
//        else if (slashr_ok){
//            int slashr_index = (int) (slashr - str);
//            return slashr_index==m_readindex-1?HttpHandler::LINE_OPEN:HttpHandler::LINE_BAD;
//        }
//        else {
//            return HttpHandler::LINE_OPEN;
//        }
//    }
//
//    TEST(test_parse, Testparseline) {
//        /* init log */
//#ifdef SYNLOG
//        Log::get_instance()->init(__LOGFILE__, 8192, 2000000, 0); //同步日志模型
//#endif
//#ifdef ASYNLOG
//        Log::get_instance()->init(__LOGFILE__, 8192, 2000000, 10); //异步日志模型
//#endif
//
//        /* clear socket */
////        system("/tmp/tmp.h7Dy0SaBof/test/test_epoll/close_sock.sh");
////        char temp[100];
////        string path = getcwd(temp, sizeof(temp)) ? std::string( temp ) : std::string("");
////        cout<< path <<endl;
//
//        /* test */
//        char test1[123] = "asdasdsda\r\nas";
//        char test2[123] = "asdasdsda\r";
//        char test3[123] = "asdasdsda\rdasdsda";
//        char test4[123] = "asdasdsda\n";
//        char test5[123] = "asdasdsda\n213213";
//        char test6[123] = "asdasdsda213213";
//        char test7[123] = "asdas\0dsda213213";
//        char test8[123] = "asdas\0dsda\r\n3213";
//
//        /* parse */
//        auto h = new HttpHandler();
//
//        h->set_buf(test1);
//        h->set_readidx(123);
//        auto res = h->parse_line();
//        EXPECT_EQ(res, HttpHandler::LINE_OK);
//        EXPECT_STREQ(h->get_buf(), "asdasdsda");
//
//        h->set_buf(test2);
//        h->set_readidx(10);
//        res = h->parse_line();
//        EXPECT_EQ(res, HttpHandler::LINE_OPEN);
//        EXPECT_STREQ(h->get_buf(), "asdasdsda\r");
//
//        h->set_buf(test3);
//        h->set_readidx(123);
//        res = h->parse_line();
//        EXPECT_EQ(res, HttpHandler::LINE_BAD);
//        EXPECT_STREQ(h->get_buf(), "asdasdsda\rdasdsda");
//
//        h->set_buf(test4);
//        h->set_readidx(10);
//        res = h->parse_line();
//        EXPECT_EQ(res, HttpHandler::LINE_BAD);
//        EXPECT_STREQ(h->get_buf(), "asdasdsda\n");
//
//        h->set_buf(test5);
//        h->set_readidx(123);
//        res = h->parse_line();
//        EXPECT_EQ(res, HttpHandler::LINE_BAD);
//        EXPECT_STREQ(h->get_buf(), "asdasdsda\n213213");
//
//        h->set_buf(test6);
//        h->set_readidx(123);
//        res = h->parse_line();
//        EXPECT_EQ(res, HttpHandler::LINE_OPEN);
//        EXPECT_STREQ(h->get_buf(), "asdasdsda213213");
//
//        h->set_buf(test7);
//        h->set_readidx(123);
//        res = h->parse_line();
//        EXPECT_EQ(res, HttpHandler::LINE_OPEN);
//        EXPECT_STREQ(h->get_buf(), "asdas\0dsda213213");
//
//        h->set_buf(test8);
//        h->set_readidx(123);
//        res = h->parse_line();
//        EXPECT_EQ(res, HttpHandler::LINE_OPEN);
//        EXPECT_STREQ(h->get_buf(), "asdas\0dsda\0\0 3213");
//
//    }
//
//    HttpHandler::HTTP_STATE parse_request_(char *text){
//        char* m_url, *m_version;
//        HttpHandler::METHOD m_method;
//        int m_cgi = 1;
//
//        /* go locate m_url */
//        m_url = strpbrk(text, " ");
//        if (!m_url){
//            return HttpHandler::ERROR_FORMAT;
//        }
//        *m_url++ = '\0';
//
//        /* get method */
//        char *method = text;
//        if (strcasecmp(method, "GET") == 0)
//            m_method = HttpHandler::GET;
//        else if (strcasecmp(method, "POST") == 0){
//            m_method = HttpHandler::POST;
//            m_cgi = 1;
//        }
//        else if (strcasecmp(method, "HEAD") == 0){
//            m_method = HttpHandler::HEAD;
//        }
//        else
//            return HttpHandler::ERROR_FORMAT;
//        fprintf(stdout, "%s\n", method);
//
//        /* go locate version */
//        m_version = strpbrk(m_url, " ");
//        if (!m_version) {
//            return HttpHandler::ERROR_FORMAT;
//        }
//        *m_version++ = '\0';
//
//        /* get url */
//        if (strncasecmp(m_url, "http://", 7) == 0){
//            m_url += 7;
//            m_url = strchr(m_url, '/');
//        }
//        if (strncasecmp(m_url, "https://", 8) == 0){
//            m_url += 8;
//            m_url = strchr(m_url, '/');
//        }
//        if (!m_url)
//            return HttpHandler::ERROR_FORMAT;
//
//        /* get version */
//        if (strcasecmp(m_version, "HTTP/1.1") != 0)
//            return HttpHandler::ERROR_FORMAT;
//        fprintf(stdout, "%s\n", m_version);
//
//
//        /* when m_url == "/" */
//        if (strlen(m_url) == 1)
//            strcat(m_url, "judge.html");
//        fprintf(stdout, "%s\n", m_url);
//
//        /* go for next */
////        m_check_state = HttpHandler::CHECK_STATE_HEADER;
//        return HttpHandler::PARSERING;
//    }
//
//    TEST(test_parse, Testrequest) {
//        /* init log */
//#ifdef SYNLOG
//        Log::get_instance()->init(__LOGFILE__, 8192, 2000000, 0); //同步日志模型
//#endif
//#ifdef ASYNLOG
//        Log::get_instance()->init(__LOGFILE__, 8192, 2000000, 10); //异步日志模型
//#endif
//
//        /* clear socket */
////        system("/tmp/tmp.h7Dy0SaBof/test/test_epoll/close_sock.sh");
////        char temp[100];
////        string path = getcwd(temp, sizeof(temp)) ? std::string( temp ) : std::string("");
////        cout<< path <<endl;
//
//        /* test */
//        char test1[123] = "GET / HTTP/1.1";
//        char test2[123] = "GET https://www.bilibili.com/bangumi/play/ss25474?t=1315 HTTP/1.1";
//        char test3[123] = "GET https://www.w3.org/Protocols/rfc2616/rfc2616-sec5.html#sec5.1 HTTP/1.1";
//        char test4[123] = "GET 123ds a21 HTTP/1.1";
//        char test5[123] = "GET / a21 HTTP/1.1";
//
//        /* parse */
//        auto h = new HttpHandler();
//
//        h->set_buf(test1);
//        auto res = h->parse_request(test1);
//        EXPECT_EQ(res, HttpHandler::PARSERING);
//        EXPECT_EQ(h->get_my_method(), HttpHandler::GET);
//        EXPECT_STREQ(h->get_my_url(), "/judge.html");
////        EXPECT_STREQ(h->get_my_version(), "HTTP/1.1");
//
//        h->set_buf(test2);
//        res = h->parse_request(test2);
//        EXPECT_EQ(res, HttpHandler::PARSERING);
//        EXPECT_EQ(h->get_my_method(), HttpHandler::GET);
//        EXPECT_STREQ(h->get_my_url(), "/bangumi/play/ss25474?t=1315");
//        EXPECT_STREQ(h->get_my_version(), "HTTP/1.1");
//
//
//        h->set_buf(test3);
//        res = h->parse_request(test3);
//        EXPECT_EQ(res, HttpHandler::PARSERING);
//        EXPECT_EQ(h->get_my_method(), HttpHandler::GET);
//        EXPECT_STREQ(h->get_my_url(), "/Protocols/rfc2616/rfc2616-sec5.html#sec5.1");
//        EXPECT_STREQ(h->get_my_version(), "HTTP/1.1");
//
//        h->set_buf(test4);
//        res = parse_request_(test4);
//        EXPECT_EQ(res, HttpHandler::ERROR_FORMAT);
//        EXPECT_EQ(h->get_my_method(), HttpHandler::GET);
////        EXPECT_STREQ(h->get_my_url(), "/bangumi/play/ss25474?t=1315");
////        EXPECT_STREQ(h->get_my_version(), "HTTP/1.1");
//
//        h->set_buf(test5);
//        res = parse_request_(test5);
//        EXPECT_EQ(res, HttpHandler::ERROR_FORMAT);
//        EXPECT_EQ(h->get_my_method(), HttpHandler::GET);
////        EXPECT_STREQ(h->get_my_url(), "/bangumi/play/ss25474?t=1315");
////        EXPECT_STREQ(h->get_my_version(), "HTTP/1.1");
//    }
//
//
//    class test{
//
//    };
//
//    TEST(test_parse, Testheaders) {
//        /* init log */
//#ifdef SYNLOG
//        Log::get_instance()->init(__LOGFILE__, 8192, 2000000, 0); //同步日志模型
//#endif
//#ifdef ASYNLOG
//        Log::get_instance()->init(__LOGFILE__, 8192, 2000000, 10); //异步日志模型
//#endif
//
//        /* clear socket */
//        system("/tmp/tmp.h7Dy0SaBof/test/test_epoll/close_sock.sh");
//        char temp[100];
//        string path = getcwd(temp, sizeof(temp)) ? std::string( temp ) : std::string("");
//
//        /* test */
//        char test1[123] = "Connection: keep-alive ";
//        char test2[123] = "Content-length: \t\t 15123 ";
//        char test3[123] = "Host: \t \t adasdaefacea \t ";
//        char test4[123] = "\0asdasd\r\n";
//        char test5[123] = "GEasdT 123ds a21 HTTP/1.1";
//        char test6[123] = "GET / a21 HTTP/1.1";
//
//
//        /* parse */
//        auto h = new HttpHandler();
//
//        h->set_buf(test1);
//        auto res = h->parse_headers(test1);
//        EXPECT_EQ(res, HttpHandler::PARSERING);
//        EXPECT_EQ(h->get_my_linger(), true);
//
//        h->set_buf(test2);
//        res = h->parse_headers(test2);
//        EXPECT_EQ(res, HttpHandler::PARSERING);
//        EXPECT_EQ(h->get_my_length(), 15123);
//
//        h->set_buf(test3);
//        res = h->parse_headers(test3);
//        EXPECT_EQ(res, HttpHandler::PARSERING);
//        EXPECT_STREQ(h->get_my_host(), "adasdaefacea");
//
////        h->set_buf(test4);
////        res = h->parse_headers(test4);
////        EXPECT_EQ(res, HttpHandler::READY_REQUEST);
//
//        h->set_buf(test5);
//        res = h->parse_headers(test5);
//        EXPECT_EQ(res, HttpHandler::PARSERING);
//
//        h->set_buf(test6);
//        res = h->parse_headers(test6);
//        EXPECT_EQ(res, HttpHandler::PARSERING);
//
//    }

    TEST(test_parse, Testall) {
        /* init log */
#ifdef SYNLOG
        Log::get_instance()->init(__LOGFILE__, 8192, 2000000, 0); //同步日志模型
#endif
#ifdef ASYNLOG
        Log::get_instance()->init(__LOGFILE__, 8192, 2000000, 10); //异步日志模型
#endif

        /* clear socket */
        system("/tmp/tmp.h7Dy0SaBof/test/test_epoll/close_sock.sh");
        char temp[100];
        string path = getcwd(temp, sizeof(temp)) ? std::string( temp ) : std::string("");

        /* test */
        char test1[123] = "GET / HTTP/1.1\r\nUser-Agent: 441UserAgent/1.0.0\r\n\r\n\0\r\n";
        char test2[123] = "Content-length: \t\t 15123 ";
        char test3[123] = "Host: \t \t adasdaefacea \t ";


        /* parse */
        auto h = new HttpHandler();

        h->set_buf(test1);
        auto res = h->parse();
        EXPECT_EQ(res, HttpHandler::FINISHED_REQUEST);
        EXPECT_EQ(h->get_my_method(), HttpHandler::GET);
        EXPECT_STREQ(h->get_my_useragent(), "441UserAgent/1.0.0");


//        h->set_buf(test2);
//        res = h->parse_headers(test2);
//        EXPECT_EQ(res, HttpHandler::PARSERING);
//        EXPECT_EQ(h->get_my_length(), 15123);
//
//        h->set_buf(test3);
//        res = h->parse_headers(test3);
//        EXPECT_EQ(res, HttpHandler::PARSERING);
//        EXPECT_STREQ(h->get_my_host(), "adasdaefacea");

//        h->set_buf(test4);
//        res = h->parse_headers(test4);
//        EXPECT_EQ(res, HttpHandler::READY_REQUEST);

//        h->set_buf(test5);
//        res = h->parse_headers(test5);
//        EXPECT_EQ(res, HttpHandler::PARSERING);
//
//        h->set_buf(test6);
//        res = h->parse_headers(test6);
//        EXPECT_EQ(res, HttpHandler::PARSERING);

    }

} // namespace cmudb
