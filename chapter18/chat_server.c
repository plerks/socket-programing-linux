#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define MAX_CLIENT 50
#define BUF_SIZE 50
int client_socks[MAX_CLIENT];
int client_count = 0;
void error_handling(char *message);
void *handle_connection(void *args);
void send_msg(char* buf, int len);
pthread_mutex_t mutex;

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
    pthread_mutex_init(&mutex, 0);
    pthread_t thread;
    while (1) {
        struct sockaddr_in client_addr;
        int addr_size;
        int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, (socklen_t *)&addr_size);
        pthread_mutex_lock(&mutex);
        client_socks[client_count++] = client_sock;
        pthread_mutex_unlock(&mutex);
        // can be optimized to use thread poll or something
        pthread_create(&thread, NULL, handle_connection, (void *)&client_sock);
        pthread_detach(thread);
    }
}

void *handle_connection(void *arg) {
    int client_sock = *(int *)arg;
    char buf[BUF_SIZE];
    int len;
    while ((len = read(client_sock, buf, BUF_SIZE)) != 0) {
        send_msg(buf, len);
    }
    pthread_mutex_lock(&mutex);
    // remove disconnected client, client_sock can be optimized to hashset, but no default hashset in c lib
    for (int i = 0; i < client_count; i++) {
        if (client_socks[i] == client_sock) {
            for (int j = i; j < client_count - 1;j++) {
                client_socks[j] = client_socks[j + 1];
            }
        }
    }
    client_count--;
    pthread_mutex_unlock(&mutex);
    close(client_sock);
    return NULL;
}

void send_msg(char *buf, int len) {
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < client_count; i++) {
        write(client_socks[i], buf, len);
    }
    pthread_mutex_unlock(&mutex);
}

void error_handling(char *message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}