// Echo server. Return the data as the client transports and store data in a temp file. Run along with chapter10/echo_client.c.
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

#define BUF_SIZE 100

void error_handling(char *message);
void read_childproc(int);

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
    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        error_handling("bind() error");
    }
    if (listen(server_sock, 5) == -1) {
        error_handling("listen() error");
    }
    
    struct sigaction act;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    act.sa_handler = read_childproc;
    // Register a callback to the os on the SIGCHLD signal. When child process exits, act.sa_handler will be called.
    sigaction(SIGCHLD, &act, 0);

    int fd[2];
    pipe(fd);
    pid_t pid = fork();
    if (pid == 0) { // child process
        /* Set that when parent process exits, child process exits. Else this write-file process still exists
        after the parent process exits. And there will be a bind() error when rerun the program.
        */
        prctl(PR_SET_PDEATHSIG,SIGKILL);
        FILE *fp = fopen("temp.txt", "wt");
        char store_buf[BUF_SIZE];
        memset(&store_buf, 0, sizeof(store_buf));
        while (1) {
            int len = read(fd[0], store_buf, BUF_SIZE);
            // puts() has the same effect as printf("%s\n",s), while fputs() does't automatically add a '\n' in the end
            // puts(store_buf);
            fwrite(store_buf, 1, len, fp);
            fflush(fp);
        }
        fclose(fp);
        return 0;
    }
    struct sockaddr_in client_addr;
    char client_data_buf[BUF_SIZE];
    while(1) {
        int addr_size = sizeof(client_addr);
        int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, (socklen_t *)&addr_size);
        char client_ip_arr[20];
        char *client_ip_ptr = inet_ntoa(client_addr.sin_addr);
        /* inet_ntoa() returns a char *, the space is probably staticly allocated inner inet_ntoa(), so
        the result should be copied before next inet_ntoa() call */
        strcpy(client_ip_arr, client_ip_ptr);
        fprintf(stdout, "client : %s:%d connected\n", client_ip_arr, client_addr.sin_port);
        // every time a new client is accepted, a new child process is created to handle the connection
        pid_t pid = fork();
        if (pid == 0) { // child process
            close(server_sock);
            int len = 0;
            /* The client_data_buf here is not a critical resource. Each child process has their own copied client_data_buf.
            They are not using the parent process' client_data_buf. */
            while ((len = read(client_sock, client_data_buf, BUF_SIZE)) != 0) {
                write(client_sock, client_data_buf, len);
                write(fd[1], client_data_buf, len);
            }
            close(client_sock);
            printf("client : %s:%d disconnected\n", client_ip_arr, client_addr.sin_port);
            return 0;
        }
        else {
            // Parent process close the file descriptor, remove reference to the socket
            close(client_sock);
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

void read_childproc(int) {
    int status;
    // get child process return state to recycle zombie process
    pid_t pid = waitpid(-1, &status, WNOHANG);
    if (WIFEXITED(status)) {
        printf("child process with pid %d normally exited with exit value: %d\n", pid, WEXITSTATUS(status));
    }
}