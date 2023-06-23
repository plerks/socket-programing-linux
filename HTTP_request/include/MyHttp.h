#ifndef HTTP_H
#define HTTP_H

struct Http {
    pthread_t (*get)(char *url, void *(*then)(void *));
    pthread_t (*post)(char *url, char *data, int length, void *(*then)(void *));
};

pthread_t get(char *url, void *(*then)(void *));

pthread_t post(char *url, char *data, int length, void *(*then)(void *));

struct Http http = {get, post};
#endif