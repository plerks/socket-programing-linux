#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include "MyHttps.h"

void *callback_get(void *body);
void *callback_post(void *body);

int main(int argc, char *argv[]) {
    char url[] = {"https://www.baidu.com/index.html"};
    pthread_t thread = http.get(url, callback_get);
    printf("thread1 running\n");

    char url2[] = {"https://localhost:44300/postdemo"};
    char data[] = {"This is data posted to the server api '/postdemo' and the response body will contain the same data."};
    pthread_t thread2 = http.post(url2, data, strlen(data), callback_post);
    printf("thread2 running\n");
    // inner get()/post(), a thread is created to perform request, wait until the thread ends
    pthread_join(thread, NULL);
    pthread_join(thread2, NULL);
}

// body points to response body
void *callback_get(void *body) {
    char *r = (char *)body;
    printf("%s\n", r);
}

void *callback_post(void *body) {
    char *r = (char *)body;
    printf("%s\n", r);
}