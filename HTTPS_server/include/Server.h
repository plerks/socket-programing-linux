#ifndef SERVER_H
#define SERVER_H

#include "Handlers.h"

void startServer(char *ip, int port);

int getContentLength(char *buf);

void my_SSL_write_and_send(struct Request *request, char *s, int len);
#endif