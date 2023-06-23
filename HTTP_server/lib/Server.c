#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include "Handlers.h"

/* let the main thread put client fd into a blocking queue and the worker threads get and handle the request. Here
simply use an array and mutex as blocking queue. Assme that the client number won't be bigger than MAX_CLIENT_SIZE. */
struct Node {
    int fd;
    struct Node *succ;
};

struct ClientFds {
    pthread_mutex_t mutex;
    struct Node *first;
    struct Node *last;
};

// is thread-safe
int clientFds_enqueue(struct ClientFds *clientFds, int fd) {
    pthread_mutex_lock(&clientFds->mutex);
    struct Node *node = malloc(sizeof(struct Node));
    node->fd = fd;
    node->succ = NULL;
    // empty queue
    if (clientFds->first == NULL) {
        clientFds->first = node;
        clientFds->last = node;
        pthread_mutex_unlock(&clientFds->mutex);
        return 0;
    }
    else {
        clientFds->last->succ = node;
        clientFds->last = node;
        pthread_mutex_unlock(&clientFds->mutex);
        return 0;
    }
}

// is thread-safe
int clientFds_dequeue(struct ClientFds *clientFds) {
    pthread_mutex_lock(&clientFds->mutex);
    // empty queue
    if (clientFds->first == NULL) {
        pthread_mutex_unlock(&clientFds->mutex);
        return -1;
    }
    struct Node *temp = clientFds->first;
    int fd = temp->fd;
    clientFds->first = temp->succ;
    if (clientFds->first == NULL) {
        clientFds->last = NULL;
    }
    pthread_mutex_unlock(&clientFds->mutex);
    free(temp);
    return fd;
}

#define BUF_SIZE 50
#define EPOLL_SIZE 50
#define MAX_HEADER_SIZE 2048
void error_handling(char *message);
void *thread_run(void *arg);
void init_clientFds(struct ClientFds *clientFds);
void *callProperHandler(int sock);
void *defaultResonse(struct Request *request);
void *sendErrorPage(struct Request *request);
void getRequestMethodAndUrl(struct Request *request, char *requestMethod, char *url);
bool endsWith(char *src, char *subString);
char *newString(char *buf, char *start, char *end);

int startServer(char *ip, int port) {
    int server_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
        error_handling("socket() error");
    }
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(inet_addr(ip));
    server_addr.sin_port = htons(port);
    int option = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &option, (socklen_t)sizeof(option));
    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        error_handling("bind() error");
    }
    if (listen(server_sock, 5) == -1) {
        error_handling("listen() error");
    }
    struct sockaddr_in client_addr;
    struct epoll_event events[EPOLL_SIZE];
    struct epoll_event event;
    event.events = EPOLLIN; // use level trigger, use edge trigger will ocassionally cause the browser keeps pending, weird.
    event.data.fd = server_sock;
    int epfd = epoll_create(EPOLL_SIZE);
    epoll_ctl(epfd, EPOLL_CTL_ADD, server_sock, &event);
    char buf[BUF_SIZE];
    long number_of_processors = sysconf(_SC_NPROCESSORS_ONLN);
    struct ClientFds *clientFds = malloc(sizeof(struct ClientFds));
    init_clientFds(clientFds);
    pthread_t thread;
    for (int i = 0; i < number_of_processors; i++) {
        pthread_create(&thread, NULL, thread_run, (void *)clientFds);
    }
    while (1) {
        int epoll_num = epoll_wait(epfd, events, EPOLL_SIZE, -1);
        // printf("epoll_num: %d\n", epoll_num);
        if (epoll_num == -1) {
            /* see <https://stackoverflow.com/questions/6870158/epoll-wait-fails-due-to-eintr-how-to-remedy-this>,
            <https://blog.csdn.net/xidomlove/article/details/8274732>:
            It seems that gdb might interrupt system call epoll_wait(). Just to recall epoll_wait() is fine.
            */
            if (errno == EINTR) {
                continue;
            }
            printf("epoll_wait() error with errno:%d\n", errno);
            break;
        }
        for (int i = 0; i < epoll_num; i++) {
            if (events[i].data.fd == server_sock) {
                int addr_size = sizeof(client_addr);
                int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_size);
                // printf("accept client_sock: %d\n", client_sock);
                int flag = fcntl(client_sock, F_GETFL | O_NONBLOCK);
                fcntl(client_sock, F_SETFL, flag); // non-blocking socket
                getpeername(client_sock, (struct sockaddr *)&client_addr, &addr_size);
                // printf("client : %s:%d connected\n", inet_ntoa(client_addr.sin_addr), client_addr.sin_port);
                event.data.fd = client_sock;
                epoll_ctl(epfd, EPOLL_CTL_ADD, client_sock, &event);
            }
            else {
                epoll_ctl(epfd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
                // printf("enqueue %d\n", events[i].data.fd);
                clientFds_enqueue(clientFds, events[i].data.fd);
            }
        }
    }
    close(server_sock);
    close(epfd);
    return 0;
}

void *thread_run(void *arg) {
    struct ClientFds *clientFds = (struct ClientFds *)arg;
    while (1) {
        int client_sock;
        client_sock = clientFds_dequeue(clientFds);
        /* if (client_sock > 0) {
            printf("dequeue client_sock: %d\n", client_sock);
        } */
        if (client_sock == -1) {
            continue;
        }
        callProperHandler(client_sock);
        // printf("close client_sock: %d\n", client_sock);
        close(client_sock);
    }
}

void init_clientFds(struct ClientFds *clientFds) {
    memset(clientFds, 0, sizeof(clientFds));
    pthread_mutex_init(&clientFds->mutex, 0);
}

void *callProperHandler(int sock) {
    char buf[MAX_HEADER_SIZE] = {0};
    int totalLen = 0;
    int len = 0;
    // receive the full header first
    while (totalLen < MAX_HEADER_SIZE && strstr(buf, "\r\n\r\n") == NULL) {
        len = read(sock, buf + totalLen, MAX_HEADER_SIZE - totalLen);
        if (len > 0) {
            totalLen += len;
            if (totalLen > MAX_HEADER_SIZE) {
                printf("the request header is too long\n");
                return NULL;
            }
        }
        else {
            if (len == 0) {
                printf("the peer closed the connection\n");
                return NULL;
            }
            else {
                // see <https://stackoverflow.com/questions/1694164/is-errno-thread-safe>, errno is thread-safe.
                if (errno == EAGAIN) {
                    continue;
                }
                else {
                    printf("read() returned %d and errno is %d\n", len, errno);
                    return NULL;
                }
            }
        }
    }
    struct Request request;
    memset(&request, 0, sizeof(request));
    request.sock = sock;
    request.buf = buf;
    request.totalLen = totalLen;
    request.requestLine = newString(buf, buf, strstr(buf, "\r\n") + strlen("\r\n"));
    request.headers = newString(buf, strstr(buf, "\r\n") + strlen("\r\n"), strstr(buf, "\r\n\r\n") + strlen("\r\n"));
    request.emptyLine = newString(buf, strstr(buf, "\r\n\r\n") + strlen("\r\n"), strstr(buf, "\r\n\r\n") + 2 * strlen("\r\n"));
    char requestMethod[MAX_HTTP_REQUEST_METHOD_LENGTH];
    char url[URL_LENGTH];
    memset(requestMethod, 0, MAX_HTTP_REQUEST_METHOD_LENGTH);
    memset(url, 0, URL_LENGTH);
    getRequestMethodAndUrl(&request, requestMethod, url);
    for (int i = 0; i < handler_num; i++) {
        if (!strcmp(requestMethod, handlers[i]->requestMethod) && !strcmp(url, handlers[i]->url)) {
            handlers[i]->handle(&request);
            freeRequest(&request);
            return NULL;
        }
    }
    defaultResonse(&request);
    freeRequest(&request);
    return NULL;
}

int getContentLength(char *buf) {
    char *s = strstr(buf, "Content-Length");
    if (s == NULL) {
        return 0;
    }
    s = strstr(s, ":");
    char numString[50] = {0};
    s = s + 1;
    for (int i = 0; s[i] != '\r'; i++) {
        numString[i] = s[i];
    }
    return atoi(numString);
}

// construct a heap String of buf[start, end)
char *newString(char *buf, char *start, char *end) {
    int length = end -start;
    char *s = (char *)malloc(length);
    memset(s, 0 , length);
    for (int i = 0; i < length; i++) {
        s[i] = *(start + i);
    }
    return s;
}

// no programer-defined logic found, try to match static files (could be optimized to cache the file content in memory for efficiency)
void *defaultResonse(struct Request *request) {
    char requestMethod[MAX_HTTP_REQUEST_METHOD_LENGTH];
    char url[URL_LENGTH];
    memset(requestMethod, 0, MAX_HTTP_REQUEST_METHOD_LENGTH);
    memset(url, 0, URL_LENGTH);
    getRequestMethodAndUrl(request, requestMethod, url);
    /* if (!strcmp(url, "/")) {
        strcpy(url, "/index.html");
    } */
    char path[URL_LENGTH] = {0};
    strcpy(path, "resources");
    strcat(path, url);
    FILE *file;
    if ((file = fopen(path, "r")) == NULL) {
        return sendErrorPage(request);
    }
    else {
        int sock = request->sock;
        char protocol[] = "HTTP/1.0 200 OK\r\n";
        char serverName[] = "Server: simple web server\r\n";
        char *contentType = "Content-type: text/plain\r\n";
        if (endsWith(url, ".html")) {
            contentType = "Content-type: text/html\r\n";
        }
        if (endsWith(url, ".css")) {
            contentType = "Content-type: text/css\r\n";
        }
        if (endsWith(url, ".js")) {
            contentType = "Content-type: application/javascript\r\n";
        }
        char emptyLine[] = "\r\n";
        /* see <https://stackoverflow.com/questions/108183/how-to-prevent-sigpipes-or-handle-them-properly>:
        If the client close the connection early (for example, use browser, reload the page and stop reloading at once),
        send() will cause a SIGPIPE signal, use MSG_NOSIGNAL to prevent send() from triggering any signal.
        */
        send(sock, protocol, strlen(protocol), MSG_NOSIGNAL);
        send(sock, serverName, strlen(serverName), MSG_NOSIGNAL);
        send(sock, contentType, strlen(contentType), MSG_NOSIGNAL);
        send(sock, emptyLine, strlen(emptyLine), MSG_NOSIGNAL);
        char buf[BUF_SIZE];
        while (fgets(buf, BUF_SIZE, file) != NULL) {
            send(sock, buf, strlen(buf), MSG_NOSIGNAL);
        }
    }
}

void *sendErrorPage(struct Request *request) {
    char requestMethod[MAX_HTTP_REQUEST_METHOD_LENGTH];
    char url[URL_LENGTH];
    memset(requestMethod, 0, MAX_HTTP_REQUEST_METHOD_LENGTH);
    memset(url, 0, URL_LENGTH);
    getRequestMethodAndUrl(request, requestMethod, url);
    char path[URL_LENGTH] = {"resources/errorpage.html"};
    FILE *file = fopen(path, "r");
    int sock = request->sock;
    char protocol[] = "HTTP/1.0 400 Bad Request\r\n";
    char serverName[] = "Server: simple web server\r\n";
    char contentType[] = "Content-type: text/html\r\n";
    char emptyLine[] = "\r\n";
    send(sock, protocol, strlen(protocol), MSG_NOSIGNAL);
    send(sock, serverName, strlen(serverName), MSG_NOSIGNAL);
    send(sock, contentType, strlen(contentType), MSG_NOSIGNAL);
    send(sock, emptyLine, strlen(emptyLine), MSG_NOSIGNAL);
    char buf[BUF_SIZE];
    while (fgets(buf, BUF_SIZE, file) != NULL) {
        send(sock, buf, strlen(buf), MSG_NOSIGNAL);
    }
}

void getRequestMethodAndUrl(struct Request *request, char *requestMethod, char *url) {
    char *s = request->requestLine;
    for (int i = 0; (*s) != ' ';i++) {
        requestMethod[i] = *s;
        s++;
    }
    s++;
    for (int i = 0; (*s) != ' ';i++) {
        url[i] = *s;
        s++;
    }
}

bool endsWith(char *src, char *subString) {
    if (strlen(src) < strlen(subString)) {
        return false;
    }
    else {
        for (int i = 0; strlen(subString) - i > 0; i++) {
            if (src[strlen(src) - i] != subString[strlen(subString) -i]) {
                return false;
            }
        }
        return true;
    }
}

void error_handling(char *message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}