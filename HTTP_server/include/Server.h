#ifndef SERVER_H
#define SERVER_H

#include "Handlers.h"

void startServer(char *ip, int port);

int getContentLength(char *buf);
#endif