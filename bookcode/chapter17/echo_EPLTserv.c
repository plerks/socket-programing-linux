#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#define BUF_SIZE 50
#define EPOLL_SIZE 50
void error_handling(char *message);

int main(int argc, char *argv[]) {
    // if not given a port number argument, the server serves at port 9190 by default
    if (argc != 2) {
        argv[1] = "9190";
    }
    int server_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
        error_handling("socket() error");
    }
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(atoi(argv[1]));
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
    event.events = EPOLLIN; // default mode for epoll is level trigger
    event.data.fd = server_sock;
    int epfd = epoll_create(EPOLL_SIZE);
    epoll_ctl(epfd, EPOLL_CTL_ADD, server_sock, &event);
    char buf[BUF_SIZE];
    while (1) {
        int epoll_num = epoll_wait(epfd, events, EPOLL_SIZE, -1);
        if (epoll_num == -1) {
            fputs("epoll_wait() error\n", stdout);
            break;
        }
        // fputs("epoll triggered\n", stdout);
        for (int i = 0; i < epoll_num; i++) {
            if (events[i].data.fd == server_sock) {
                int addr_size = sizeof(client_addr);
                int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_size);
                getpeername(client_sock, (struct sockaddr *)&client_addr, &addr_size);
                printf("client : %s:%d connected\n", inet_ntoa(client_addr.sin_addr), client_addr.sin_port);
                event.data.fd = client_sock;
                epoll_ctl(epfd, EPOLL_CTL_ADD, client_sock, &event);
            }
            else {
                int len = read(events[i].data.fd, buf, BUF_SIZE);
                if (len == 0) {
                    int addr_size = sizeof(client_addr);
                    getpeername(events[i].data.fd, (struct sockaddr *)&client_addr, &addr_size);
                    epoll_ctl(epfd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
                    close(events[i].data.fd);
                    printf("client : %s:%d disconnected\n", inet_ntoa(client_addr.sin_addr), client_addr.sin_port);
                }
                else {
                    write(events[i].data.fd, buf, len);
                }
            }
        }
    }
    close(server_sock);
    close(epfd);
    return 0;
}

void error_handling(char *message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}