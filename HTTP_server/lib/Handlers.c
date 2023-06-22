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
    free(request->requestLine);
    free(request->headers);
    free(request->emptyLine);
}