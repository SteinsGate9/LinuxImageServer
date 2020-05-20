#include <mysql/mysql.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <list>
#include <pthread.h>
#include <iostream>

#include "sql_connection_pool.h"
#include "logger.h"

using namespace std;

connectionPool *connectionPool::connPool = NULL;
static pthread_mutex_t mutex;

//初始化mutex
connectionPool::connectionPool()
{
	pthread_mutex_init(&mutex, NULL);
}

//构造初始化
connectionPool::connectionPool(string url, string User, string PassWord, string DBName, int Port, unsigned int MaxConn)
{
	this->url = url;
	this->Port = Port;
	this->User = User;
	this->PassWord = PassWord;
	this->DatabaseName = DBName;

	pthread_mutex_lock(&lock);
	for (int i = 0; i < (int)MaxConn; i++)
	{
		MYSQL *con = NULL;
		con = mysql_init(con);

		if (con == NULL){
		    CONSOLE_LOG_ERROR("mysql init %s", mysql_error(con));
		}
		con = mysql_real_connect(con, url.c_str(), User.c_str(), PassWord.c_str(), DBName.c_str(), Port, NULL, 0);

		if (con == NULL){
            CONSOLE_LOG_ERROR("mysql connection %s", mysql_error(con));
		}
		_conn_list.push_back(con);
		++FreeConn;
	}

	this->MaxConn = MaxConn;
	this->CurConn = 0;
	pthread_mutex_unlock(&lock);
}

//获得实例，只会有一个
connectionPool *connectionPool::get_instance(string url, string User, string PassWord, string DBName, int Port, unsigned int MaxConn)
{
	//先判断是否为空，若为空则创建，否则直接返回现有
	if (connPool == NULL)
	{
		pthread_mutex_lock(&mutex);
		if (connPool == NULL)
		{
			connPool = new connectionPool(url, User, PassWord, DBName, Port, MaxConn);
		}
		pthread_mutex_unlock(&mutex);
	}
	return connPool;
}

//当有请求时，从数据库连接池中返回一个可用连接，更新使用和空闲连接数
MYSQL *connectionPool::get_connection()
{
	MYSQL *con = NULL;
	pthread_mutex_lock(&lock);
	//reserve.wait();
	if (_conn_list.size() > 0)
	{
		con = _conn_list.front();
		_conn_list.pop_front();

		--FreeConn;
		++CurConn;

		pthread_mutex_unlock(&lock);
		return con;
	}
	pthread_mutex_unlock(&lock);
	return NULL;
}

//释放当前使用的连接
bool connectionPool::release_connection(MYSQL *con)
{
	pthread_mutex_lock(&lock);
	if (con != NULL)
	{
		_conn_list.push_back(con);
		++FreeConn;
		--CurConn;

		pthread_mutex_unlock(&lock);
		//reserve.post();
		return true;
	}
	pthread_mutex_unlock(&lock);
	return false;
}

//销毁数据库连接池
void connectionPool::_destroy_pool(){
	pthread_mutex_lock(&lock);
	if (_conn_list.size() > 0)
	{
		list<MYSQL *>::iterator it;
		for (it = _conn_list.begin(); it != _conn_list.end(); ++it)
		{
			MYSQL *con = *it;
			mysql_close(con);
		}
		CurConn = 0;
		FreeConn = 0;
		_conn_list.clear();
		pthread_mutex_unlock(&lock);
	}
	pthread_mutex_unlock(&lock);
}

connectionPool::~connectionPool()
{
	_destroy_pool();
}
