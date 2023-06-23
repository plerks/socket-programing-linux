#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "Server.h"

void *getDemoHandleFunc(struct Request *request);
void *postDemoHandleFunc(struct Request *request);

int main(int argc, char *argv) {
    struct Handler *demoHandler_get = (struct Handler *)malloc(sizeof(struct Handler));
    memset(demoHandler_get, 0, sizeof(struct Handler));
    strcpy(demoHandler_get->requestMethod, "GET");
    strcpy(demoHandler_get->url, "/getdemo");
    demoHandler_get->handle = getDemoHandleFunc;
    addHandler(demoHandler_get);

    struct Handler *demoHandler_post = (struct Handler *)malloc(sizeof(struct Handler));
    memset(demoHandler_post, 0, sizeof(struct Handler));
    strcpy(demoHandler_post->requestMethod, "POST");
    strcpy(demoHandler_post->url, "/postdemo");
    demoHandler_post->handle = postDemoHandleFunc;
    addHandler(demoHandler_post);

    startServer("0.0.0.0", 8000);
}

void *getDemoHandleFunc(struct Request *request) {
    int sock = request->sock;
    char protocol[] = "HTTP/1.0 200 OK\r\n";
    char serverName[] = "Server: simple web server\r\n";
    /* If set Content-type as text/plain, for this response body, Chrome will decide to download it.
    I expected that Chrome would just display it as text. See:
    <https://stackoverflow.com/questions/71345343/why-do-some-images-automatically-download-when-their-url-is-visited-and-others-n>
    <https://stackoverflow.com/questions/9660514/content-type-of-text-plain-causes-browser-to-download-of-file> */
    char contentType[] = "Content-type: text/html\r\n";
    char emptyLine[] = "\r\n";
    write(sock, protocol, strlen(protocol));
    write(sock, serverName, strlen(serverName));
    write(sock, contentType, strlen(contentType));
    write(sock, emptyLine, strlen(emptyLine));
    char buf[] = {"This is a GET request demo response."};
    write(sock, buf, strlen(buf));
}

// return the post request body, assume that the post request has header: Content-Length to tell where the request body ends
void *postDemoHandleFunc(struct Request *request) {
    int sock = request->sock;
    char protocol[] = "HTTP/1.0 200 OK\r\n";
    char serverName[] = "Server: simple web server\r\n";
    char contentType[] = "Content-type: text/html\r\n";
    char emptyLine[] = "\r\n";
    write(sock, protocol, strlen(protocol));
    write(sock, serverName, strlen(serverName));
    write(sock, contentType, strlen(contentType));
    write(sock, emptyLine, strlen(emptyLine));
    int contentLength = getContentLength(request->headers);
    int len = 0;
    int totalLen = 0; // total body data received
    int buf_size = 50;
    char buf[buf_size];
    // Now request->buf may have received part of the post request body data.
    int receivedBodyLength = request->totalLen - (strstr(request->buf, "\r\n\r\n") + strlen("\r\n\r\n") - request->buf);
    /* printf("request->totalLen: %d\n", request->totalLen);
    printf("receivedBodyLength: %d\n", receivedBodyLength);
    printf("--------request->buf--------: \n%s\n", request->buf);
    printf("--------request->buf-------- ends\n"); */
    write(sock, strstr(request->buf, "\r\n\r\n") + strlen("\r\n\r\n"), receivedBodyLength);
    totalLen += receivedBodyLength;
    while (totalLen < contentLength) {
        len = read(sock, buf, buf_size);
        if (len < 0) {
            if (errno == EAGAIN) {
                continue;
            }
            else {
                printf("read() returned %d and errno is %d\n", len, errno);
                break;
            }
        }
        totalLen += len;
        write(sock, buf, len);
    }
}