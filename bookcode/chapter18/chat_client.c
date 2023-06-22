#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <arpa/inet.h>

#define MAX_CLIENT 50
#define BUF_SIZE 50
#define NAME_ARR_SIZE 6
#define MAX_SENTENCE_HEADER_SIZE 50
char name[NAME_ARR_SIZE];
void error_handling(char *message);
char *get_random_name(char *name);
int max(int a, int b);
void *send_msg(void *arg);
void *recv_msg(void *arg);

int main(int argc, char *argv[])
{
    if (argc != 3) {
        argv[1] = "127.0.0.1";
        argv[2] = "9190";
    }
    get_random_name(name);
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        error_handling("socket() error");
    }
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    server_addr.sin_port = htons(atoi(argv[2]));
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        error_handling("connect() error");
    }
    pthread_t send_thread, recv_thread;
    pthread_create(&send_thread, NULL, send_msg, (void *)&sock);
    pthread_create(&recv_thread, NULL, recv_msg, (void *)&sock);
    fputs("enter message, Q/q to quit:\n", stdout);
    void *send_thread_return;
    void *recv_thread_return;
    pthread_join(send_thread, &send_thread_return);
    pthread_join(recv_thread, &recv_thread_return);
    close(sock);
    return 0;
}

void error_handling(char *message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

char *get_random_name(char *name) {
    srand((unsigned)time(NULL));
    int i;
    for (i = 0; i < max(3, rand() % NAME_ARR_SIZE); i++) {
        name[i] = (rand() % ('z' - 'a' + 1)) + 'a';
    }
    name[i] = '\0';
    return name;
}

int max(int a, int b) {
    return a > b ? a : b;
}

void *send_msg(void *arg) {
    int sock = *(int *)arg;
    char msg[BUF_SIZE];
    while (1) {
        fgets(msg, BUF_SIZE, stdin);
        if (!strcmp(msg, "Q\n") || !strcmp(msg, "q\n")) {
            close(sock);
            exit(0);
        }
        char name_msg[MAX_SENTENCE_HEADER_SIZE + BUF_SIZE];
        sprintf(name_msg, "[%s(random name)]'s message: %s", name, msg);
        write(sock, name_msg, strlen(name_msg));
    }
    return NULL;
}

void *recv_msg(void *arg) {
    int sock = *(int *)arg;
    char name_msg[MAX_SENTENCE_HEADER_SIZE + BUF_SIZE];
    int len;
    while ((len = read(sock, name_msg, MAX_SENTENCE_HEADER_SIZE + BUF_SIZE - 1)) > 0) {
        name_msg[len] = '\0';
        fputs(name_msg, stdout);
    }
    return NULL;
}