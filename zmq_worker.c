#include <zmq.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>
#include <ctype.h>



#define MAX_MSG_LEN 1500

void normalize_word(char *word) {
    for (int i = 0; word[i]; i++) {
        word[i] = tolower(word[i]);
    }
}

void map_words(const char *text, char *result) {
    char *token;
    char buffer[MAX_MSG_LEN] = "";
    char *copy = strdup(text);
    token = strtok(copy, " .,!?;:\n\t");

    while (token) {
        normalize_word(token);
        strcat(buffer, token);
        strcat(buffer, "1 ");
        token = strtok(NULL, " .,!?;:\n\t");
    }
    free(copy);
    strcpy(result, buffer);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <worker port>\n", argv[0]);
        return 1;
    }

    char endpoint[30];
    snprintf(endpoint, sizeof(endpoint), "tcp://*:%s", argv[1]);

    void *context = zmq_ctx_new();
    void *responder = zmq_socket(context, ZMQ_REP);
    zmq_bind(responder, endpoint);

    while (1) {
        char buffer[MAX_MSG_LEN];
        zmq_recv(responder, buffer, MAX_MSG_LEN, 0);
        buffer[strlen(buffer)] = '\0';

        if (strncmp(buffer, "map", 3) == 0) {
            char result[MAX_MSG_LEN];
            map_words(buffer + 3, result);
            zmq_send(responder, result, strlen(result), 0);
        } else if (strncmp(buffer, "rip", 3) == 0) {
            zmq_send(responder, "rip", 3, 0);
            break;
        }
    }

    zmq_close(responder);
    zmq_ctx_destroy(context);
    return 0;
}
