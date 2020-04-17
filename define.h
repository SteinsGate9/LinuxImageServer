//
// Created by huangbenson on 4/17/20.
//

#ifndef LINUXSERVER_DEFINE_H
#define LINUXSERVER_DEFINE_H

#define SYNSQL          //同步数据库校验
//#define CGISQLPOOL    //CGI数据库校验
//#define CGISQL        //CGI SQL

#define SYNLOG          //同步写日志
//#define ASYNLOG       //异步写日志

#define PRINTERROR(x) printf("%s, LINE:%d, FUNCTION:%s", (x), (__LINE__), (__FUNCTION__))


#endif //LINUXSERVER_DEFINE_H
