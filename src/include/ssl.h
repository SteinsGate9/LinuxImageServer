/********************************************
 * Content: 
 * Author: by shichenh.
 * Date:  on 2020-05-21.
********************************************/
#ifndef LINUXIMAGESERVER_SSL_H
#define LINUXIMAGESERVER_SSL_H
#include <alloca.h>
#include <string.h>

#include <sys/types.h>
#include <sys/uio.h>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rsa.h>
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "logger.h"

SSL_CTX *ssl_init(char*, char*);
ssize_t SSL_writev (SSL *ssl, const struct iovec *vector, int count);

#endif //LINUXIMAGESERVER_SSL_H
