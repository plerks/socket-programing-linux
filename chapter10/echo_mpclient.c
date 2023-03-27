// Echo client. Run along with chapter11/echo_storeserv.c.
#include <stdio.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUF_SIZE 30
void error_handling(char *message);
void write_routine(int sock, char *buf);
void read_routine(int sock, char *buf);

int main(int argc, char const *argv[]) {
    // if not given server ip and port number, the client connect to 127.0.0.1:9190 by default
    if (argc != 3) {
        argv[1] = "127.0.0.1";
        argv[2] = "9190";
    }
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    server_addr.sin_port = htons(atoi(argv[2]));
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0) {
        error_handling("connect() error");
    }
    /* fork to enable that writing message to the server and reading the echo from the server can be done meantime,
    else the code would be like write() and read() alternately. */
    pid_t pid = fork();
    char buf[BUF_SIZE];
    // The buf here is not a critical resource. The parent and child process seperately have their own buf.
    if (pid == 0) {
        write_routine(sock, buf);
    }
    else {
        read_routine(sock, buf);
    }
    close(sock);
    // No need to use sigaction() and waitpid(). Unlike server, at last all processes will exit and the resources get recycled by os.
    return 0;
}

void write_routine(int sock, char *buf) {
    fputs("Type to get the echo:\n", stdout);
    while(1) {
        /* fgets() will ensure that the string ends with '\0'. fgets() reserve the '\n' while gets() not.
        However, gets(char *s) lack buffer overrun checking. */
        fgets(buf, BUF_SIZE, stdin);
        if (!strcmp(buf, "Q\n") || !strcmp(buf, "q\n")) {
            /* Shutdown write, and the server's read() return 0 and server code will close the connection. And then the read_routine()'s
            read() function can return 0 so that the read process can exit. Without this shutdown(sock, SHUT_WR), the read
            process will not exit even after typing 'q'. It seems that when multiple file descriptor refering one socket,
            only when all file descriptors get closed by calling close(), then the connection automatically get closed by os. However,
            shutdown() perhaps directly cause os sending fin packages to the other side to end the connection. */
            shutdown(sock, SHUT_WR);
            return;
        }
        write(sock, buf, strlen(buf));
    }
}

void read_routine(int sock, char *buf) {
    while (1) {
        /* Read at most BUF_SIZE-1 chars and remove the last '\n'. The original book here is BUF_SIZE 
        and set buf[len] = '\0', which may lead to out of range. */
        int len = read(sock, buf, BUF_SIZE - 1);
        buf[len] = '\0';
        if (len > 0 && buf[len - 1] == '\n') {
            buf[len - 1] = '\0';
        }
        if (len == 0) {
            return;
        }
        printf("Message from server: %s\n", buf);
    }
    
}

void error_handling(char *message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}