/********************************************
 * Content: 
 * Author: by shichenh.
 * Date:  on 2020-05-21.
********************************************/
#include "ssl.h"


SSL_CTX *ssl_init(char* PRIVATEKEY_FILENAME, char* CERT_FILENAME) {
    SSL_CTX * ssl_context;
    SSL_load_error_strings();
    SSL_library_init();

    /* we want to use TLSv1 only */
    if ((ssl_context = SSL_CTX_new(TLSv1_server_method())) == NULL){
        CONSOLE_LOG_ERROR("%s", "Error creating SSL context.");
    }

    /* register private key */
    if (SSL_CTX_use_PrivateKey_file(ssl_context, PRIVATEKEY_FILENAME, SSL_FILETYPE_PEM) == 0){
        SSL_CTX_free(ssl_context);
        CONSOLE_LOG_ERROR("%s", "Error associating private key.");
    }

    /* register public key (certificate) */
    if (SSL_CTX_use_certificate_file(ssl_context, CERT_FILENAME, SSL_FILETYPE_PEM) == 0){
        SSL_CTX_free(ssl_context);
        CONSOLE_LOG_ERROR("%s", "Error associating certificate.");
    }

    return ssl_context;
}

ssize_t
SSL_writev (SSL *ssl, const struct iovec *vector, int count)
{
    char *buffer;
    register char *bp;
    size_t bytes, to_copy;
    int i;

    /* Find the total number of bytes to be written.  */
    bytes = 0;
    for (i = 0; i < count; ++i)
        bytes += vector[i].iov_len;

    /* Allocate a temporary buffer to hold the data.  */
    buffer = (char *) alloca (bytes);

    /* Copy the data into BUFFER.  */
    to_copy = bytes;
    bp = buffer;
    for (i = 0; i < count; ++i)
    {
#     define min_(a, b)		((a) > (b) ? (b) : (a))
        size_t copy = min_ (vector[i].iov_len, to_copy);

        memcpy ((void *) bp, (void *) vector[i].iov_base, copy);
        bp += copy;

        to_copy -= copy;
        if (to_copy == 0)
            break;
    }

    return SSL_write (ssl, buffer, bytes);
}