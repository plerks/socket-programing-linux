#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/prctl.h>

#define BUF_SIZE 50
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
    /* select() requires fd_set to indicate the fds wanted to be monitored. But once return, the given argument fd_set
    will be set as the monitor result status. So the former want-to-monitor status needs to be reserved in advance. */
    fd_set readset, cpy_readset;
    FD_ZERO(&readset);
    FD_SET(server_sock, &readset);
    int select_num = 0;
    int maxfd = server_sock;
    struct timeval timeout;
    char buf[BUF_SIZE];
    while (1) {
        cpy_readset = readset;
        /* select() may update the timeout argument to indicate how much time was left (referring to linux man page).
        So must initiate timeout variable inner the loop, otherwise timeout will continue to occur after the first timeout. */
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        select_num = select(maxfd + 1, &cpy_readset, 0, 0, &timeout);
        if (select_num == -1) {
            break;
        }
        if (select_num == 0) { // timeout
            continue;
        }
        /* maxfd monotonically increase. It can be optimized, but since changed_quantity is used to control
        the loop time, the loop does't execute too much. */
        for(int i = STDERR_FILENO + 1, changed_quantity = 0; i <= maxfd && changed_quantity <= select_num; i++) {
            if (FD_ISSET(i, &cpy_readset)) {
                changed_quantity += 1;
                if (i == server_sock) { // extract connection from server_sock
                    int addr_size = sizeof(server_addr);
                    int client_sock = accept(server_sock, (struct sockaddr *)&server_addr, (socklen_t *)&addr_size);
                    if (client_sock > maxfd) {
                        maxfd = client_sock;
                    }
                    FD_SET(client_sock, &readset);
                }
                else {
                    int len = read(i, buf, BUF_SIZE);
                    if (len == 0) {
                        FD_CLR(i, &readset);
                        printf("client %d closed connection \n", i);
                        close(i);
                    }
                    else if (len > 0) {
                        write(i, buf, len);
                    }
                }
            }
        }
    }
    close(server_sock);
    return 0;
}

void error_handling(char *message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}