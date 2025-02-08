#include <zmq.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>

#define MAX_MSG_LEN 1497

// Funktion zur Normalisierung der Wörter (Kleinschreibung)
void normalize_word(char *word) {
    for (int i = 0; word[i]; i++) {
        word[i] = tolower((unsigned char) word[i]);
    }
}

// Map-Funktion: Erzeugt Key-Value-Paare aus dem Eingabetext
void map(const char *text, char *output) {
    char *token = strtok(strdup(text), " ");
    output[0] = '\0';
    while (token) {
        normalize_word(token);
        strcat(output, token);
        strcat(output, "1");
        token = strtok(NULL, " ");
    }
    free(token);
}

// Reduce-Funktion: Zählt die Häufigkeit der Wörter
void reduce(const char *input, char *output) {
    char words[MAX_MSG_LEN][50];
    int counts[MAX_MSG_LEN];
    int word_count = 0;

    char *input_copy = strdup(input);
    char *token = strtok(input_copy, "1");
    while (token) {
        normalize_word(token);
        int found = 0;
        for (int i = 0; i < word_count; i++) {
            if (strcmp(words[i], token) == 0) {
                counts[i]++;
                found = 1;
                break;
            }
        }
        if (!found) {
            strcpy(words[word_count], token);
            counts[word_count] = 1;
            word_count++;
        }
        token = strtok(NULL, "1");
    }
    free(input_copy);

    output[0] = '\0';
    for (int i = 0; i < word_count; i++) {
        char buffer[100];
        snprintf(buffer, sizeof(buffer), "%s%d", words[i], counts[i]);
        strcat(output, buffer);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        return 1;
    }

    void *context = zmq_ctx_new();
    void *responder = zmq_socket(context, ZMQ_REP);
    char endpoint[30];
    snprintf(endpoint, sizeof(endpoint), "tcp://*:%s", argv[1]);
    zmq_bind(responder, endpoint);

    while (1) {
        char buffer[MAX_MSG_LEN];
        zmq_recv(responder, buffer, MAX_MSG_LEN, 0);
        buffer[strlen(buffer)] = '\0';

        if (strncmp(buffer, "map", 3) == 0) {
            char output[MAX_MSG_LEN];
            map(buffer + 3, output);
            zmq_send(responder, output, strlen(output), 0);
        } else if (strncmp(buffer, "red", 3) == 0) {
            char output[MAX_MSG_LEN];
            reduce(buffer + 3, output);
            zmq_send(responder, output, strlen(output), 0);
        } else if (strncmp(buffer, "rip", 3) == 0) {
            zmq_send(responder, "rip", 3, 0);
            break;
        }
    }

    zmq_close(responder);
    zmq_ctx_destroy(context);
    return 0;
}
