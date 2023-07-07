#ifndef HANDLERS_H
#define HANDLERS_H

#include <openssl/ssl.h>
#include <openssl/err.h>

#define MAX_HANDLER_NUM 1024
#define MAX_HTTP_REQUEST_METHOD_LENGTH 10
#define URL_LENGTH 1024

typedef unsigned char byte;

/* The Request implementation does't contain body here. The headers are supposed to be fully received in advance by detecting the emptyLine.
About the body, the best implementation might be like:
1. The request has specified a specific Content-Length header. Then just receive the request in a buffer with pre-configured max-request-size.
2. The request has no specific Content-Length header (for example, with `Transfer-Encoding: chunked` header). Then wrap the body as an InputStream.
*/
struct Request {
    int sock;
    SSL *ssl;
    BIO *rbio;
    BIO *wbio;
    char *buf;
    int totalLen;
    char *requestLine;
    char *headers;
    char *emptyLine;
};

struct Handler {
    void *(*handle)(struct Request *);
    char requestMethod[MAX_HTTP_REQUEST_METHOD_LENGTH];
    char url[URL_LENGTH];
};

extern struct Handler *handlers[MAX_HANDLER_NUM];

extern int handler_num;

void addHandler(struct Handler *handler);

void freeRequest(struct Request *request);
#endif