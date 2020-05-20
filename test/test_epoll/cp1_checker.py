#!/usr/bin/python

from socket import *
import sys
import random
import os
import time
from tqdm import tqdm

# if len(sys.argv) < 6:
#     sys.stderr.write('Usage: %s <ip> <port> <#trials>\
#             <#writes and reads per trial>\
#             <#connections> \n' % (sys.argv[0]))
#     sys.exit(1)

serverHost = gethostbyname("172.19.0.2")
serverPort = int(1234)
numTrials = int(1)
numWritesReads = int(sys.argv[1])
numConnections = int(sys.argv[1])

if numConnections < numWritesReads:
    sys.stderr.write('<#connections> should be greater than or equal to <#writes and reads per trial>\n')
    sys.exit(1)

socketList = []

RECV_EACH_TIMEOUT = 0.1
RECV_TOTAL_TIMEOUT = 1


for i in tqdm(range(numConnections)):
    s = socket(AF_INET, SOCK_STREAM)
    s.connect((serverHost, serverPort))
    socketList.append(s)
print("connection done")


GOOD_REQUESTS = ['GET / HTTP/1.1\r\nUser-Agent: 441UserAgent/1.0.0\r\n\r\n']

BAD_REQUESTS = [
    '1dddddddddddasdasdasdasddasdsaaaaaaaaaaaaaaaaa\r\n',
    '456asdddddddddddddddddaddddddddddddddddddddddd\r\n',
    '78dddddddddddddddddddsddddddddddddddddddddddd9\r\n'
]
# GOOD_RESPONSE = ['HTTP/1.1 200 OK\r\nContent-Length:586\r\nConnection:keep-alive\r\n\r\n<!DOCTYPE html>\n<html>\n    <head>\\n        <meta charset="UTF-8">\\n        <title>WebServer</title>\\n    </head>\\n    <body>\\n    <br/>\\n    <br/>\\n    <div align="center"><font size="5"> <strong>\\u6b22\\u8fce\\u8bbf\\u95ee</strong></font></div>\\n\\t<br/>\\n\\t\\t<br/>\\n\\t\\t<form action="0" method="post">\\n \\t\\t\\t<div align="center"><button type="submit">\\u65b0\\u7528\\u6237</button></div>\\n                </form>\\n\\t\\t<br/>\\n                <form action="1" method="post">\\n                        <div align="center"><button type="submit" >\\u5df2\\u6709\\u8d26\\u53f7</button></div>\\n                </form>\\n\\t\\t\\n\\t\\t\\n        </div>\\n    </body>\\n</html>\\n\\n\\n']

# BAD_REQUESTS = [
#     'GET\r / HTTP/1.1\r\nUser-Agent: 441UserAgent/1.0.0\r\n\r\n', # Extra CR
#     'GET / HTTP/1.1\nUser-Agent: 441UserAgent/1.0.0\r\n\r\n',     # Missing CR
#     'GET / HTTP/1.1\rUser-Agent: 441UserAgent/1.0.0\r\n\r\n',     # Missing LF
# ]

BAD_REQUEST_RESPONSE = 'HTTP/1.1 400 Bad Request\r\n'

q = 0
for i in range(numTrials):
    socketSubset = []
    randomData = []
    randomLen = []
    socketSubset = random.sample(socketList, numConnections)
    for j in tqdm(range(numWritesReads)):
        random_index = 0
        # random_index = random.randrange(len(GOOD_REQUESTS) + len(BAD_REQUESTS))
        if random_index < len(GOOD_REQUESTS):
            random_string = GOOD_REQUESTS[random_index]
            randomLen.append(586)
            randomData.append(random_string)
        else:
            random_string = BAD_REQUESTS[random_index - len(GOOD_REQUESTS)]
            randomLen.append(len(BAD_REQUEST_RESPONSE))
            randomData.append(BAD_REQUEST_RESPONSE)
        socketSubset[j].send(random_string.encode())
    print("send done")
    for j in tqdm(range(numWritesReads)):
        try:
            data = socketSubset[j].recv(626)
            start_time = time.time()
            while True:
                if len(data) == randomLen[j]:
                    break
                socketSubset[j].settimeout(RECV_EACH_TIMEOUT)
                data += socketSubset[j].recv(randomLen[j])
                if time.time() - start_time > RECV_TOTAL_TIMEOUT:
                    break
        except ConnectionResetError:
            pass
        except timeout:
            pass

        # if data.decode() == randomData[j]:
        # print("datasent: ", randomData[j].encode('unicode_escape'))
        # print("dataget: ", data.decode().encode('unicode_escape'))

        if len(data.decode()) == 626:
            q += 1

            # print("failed")
            # sys.stderr.write("Error: Data received is not the same as sent! \n")
            # sys.exit(1)
        else:
            pass
print("success rate:", q, "/", numWritesReads*numTrials)


for i in range(numConnections):
    socketList[i].close()

print("Success!")
