all_files = main.cpp \
./CGImysql/sql_connection_pool.cpp ./CGImysql/sql_connection_pool.h\
./http/http_conn.cpp ./http/http_conn.h\
./lock/locker.h ./lock/locker.cpp\
./log/log.h ./log/log.cpp\
./threadpool/threadpool.h\
 ./log/block_queue.h

CGISQL.cgi:./CGImysql/sign.cpp ./CGImysql/sql_connection_pool.cpp ./CGImysql/sql_connection_pool.h
	g++ -o ./root/CGISQL.cgi ./CGImysql/sign.cpp ./CGImysql/sql_connection_pool.cpp ./CGImysql/sql_connection_pool.h -lmysqlclient

server: ${all_files}
	g++ -o server ${all_files} -lpthread -lmysqlclient

clean:
	rm  -r server
	rm  -r ./root/CGISQL.cgi

