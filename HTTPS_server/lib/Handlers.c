#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Handlers.h"

char *requestToString(struct Request *request) {
    // no need for request to toString() for now
    return NULL;
}

// should be optimized as arraylist, but no default arraylist in c
struct Handler *handlers[MAX_HANDLER_NUM];

int handler_num = 0;

void addHandler(struct Handler *handler) {
    if (handler_num >= MAX_HANDLER_NUM) {
        printf("handle number exceeded\n");
        return;
    }
    handlers[handler_num++] = handler;
}

void freeRequest(struct Request *request) {
    SSL_shutdown(request->ssl);
    /* see <https://www.openssl.org/docs/man1.1.1/man3/SSL_free.html>:
    SSL_free() also calls the free()ing procedures for indirectly affected items, if applicable: the buffering BIO, 
    the read and write BIOs, cipher lists specially created for this ssl, the SSL_SESSION. Do not explicitly free these 
    indirectly freed up items before or after calling SSL_free(), as trying to free things twice may lead to program failure.
    */
    SSL_free(request->ssl);
    free(request->requestLine);
    free(request->headers);
    free(request->emptyLine);
}