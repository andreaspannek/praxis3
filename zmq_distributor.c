#include <zmq.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>


#define MAX_MSG_LEN 1500

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <file.txt> <worker port 1> <worker port 2> ...\n", argv[0]);
        return 1;
    }

    FILE *file = fopen(argv[1], "r");
    if (!file) {
        perror("Fehler beim Öffnen der Datei");
        return 1;
    }

    char text[MAX_MSG_LEN];
    fread(text, 1, sizeof(text) - 1, file);
    fclose(file);
    text[strlen(text)] = '\0';

    void *context = zmq_ctx_new();
    void *requester = zmq_socket(context, ZMQ_REQ);

    for (int i = 2; i < argc; i++) {
        char endpoint[30];
        snprintf(endpoint, sizeof(endpoint), "tcp://localhost:%s", argv[i]);
        zmq_connect(requester, endpoint);
    }

    char map_request[MAX_MSG_LEN];
    snprintf(map_request, sizeof(map_request), "map%s", text);
    zmq_send(requester, map_request, strlen(map_request), 0);

    char map_result[MAX_MSG_LEN];
    zmq_recv(requester, map_result, MAX_MSG_LEN, 0);
    printf("Map-Result: %s\n", map_result);

    zmq_send(requester, "red", 3, 0);
    char reduce_result[MAX_MSG_LEN];
    zmq_recv(requester, reduce_result, MAX_MSG_LEN, 0);
    printf("Reduce-Result: %s\n", reduce_result);

    for (int i = 2; i < argc; i++) {
        zmq_send(requester, "rip", 3, 0);
        char rip_response[10];
        zmq_recv(requester, rip_response, 10, 0);
    }

    zmq_close(requester);
    zmq_ctx_destroy(context);
    return 0;
}

