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

int main(int argc, char *argv[]) {
    /* if not given server ip and port number, the client connects to 127.0.0.1:9190 by default.
    In case argv's length is less than 3, argvCopy is used as argv if argc != 3(to prevent ArrayIndexOutOfBounds,
    despite no exception occurred without argvCopy). argvCopy must be declared outside the "if" scope else
    there will be a segmentation fault at inet_addr() but htons(atoi(argv[2])) is fine, weired. Maybe the compiler
    chose to release the memory of argvCopy[1] after the "if" scope?
    Besides, the corresponding code on windows ran normally without any exception at inet_addr(), no need to move declaration of
    argvCopy out of the "if" scope, may because of the difference of implementation on compiler. */
    char* argvCopy[3];
    if (argc != 3) {
        argv = argvCopy;
        argvCopy[1] = "127.0.0.1";
        argvCopy[2] = "9190";
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
    fputs("Type to get the echo, Q/q to exit:\n", stdout);
    while(1) {
        /* fgets() will ensure that the string ends with '\0'. fgets() reserve the '\n' while gets() not.
        However, gets(char *s) lack buffer overrun checking. */
        fgets(buf, BUF_SIZE, stdin);
        if (!strcmp(buf, "Q\n") || !strcmp(buf, "q\n")) {
            /* Shutdown write, and the server's read() return 0 and server code will close the connection. And then the read_routine()'s
            read() function can return 0 so that the read process can exit. Without this shutdown(sock, SHUT_WR), the read
            process will not exit even after typing 'q'. It seems that when multiple file descriptor refering one socket,
            only when all file descriptors get closed by calling close(), then the connection automatically get closed by os. However,
            shutdown() perhaps directly cause os sending fin package to the other side to end the connection.
            
            About this, I made some note at
            https://github.com/plerks/mixed-tech-notes/blob/main/%E8%AE%A1%E7%AE%97%E6%9C%BA%E7%BD%91%E7%BB%9C%E7%
            9B%B8%E5%85%B3/%E5%85%B3%E4%BA%8ETCP%E4%B8%89%E6%AC%A1%E6%8F%A1%E6%89%8B%E4%B8%8E%E5%9B%9B%E6%AC%A1%E6
            %8F%A1%E6%89%8B.md
            
            A brief conclusion is that close() decreases socket's reference count by one. And if to zero, the connection gets
            closed and socket gets destroyed. However, shutdown() closes given direction's connection and shutdown(fd, SHUT_WR)
            send fin package to the other. */
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