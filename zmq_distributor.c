#include <zmq.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define MAX_MSG_LEN 1497

int main(int argc, char *argv[]) {
    if (argc < 3) {
        return 1;
    }

    FILE *file = fopen(argv[1], "r");
    if (!file) {
        perror("Fehler beim Öffnen der Datei");
        return 1;
    }

    char text[MAX_MSG_LEN] = {0};  // Puffer mit 0 initialisieren
    size_t read_bytes = fread(text, 1, sizeof(text) - 1, file);
    fclose(file);
    text[read_bytes] = '\0';

    void *context = zmq_ctx_new();
    void *requester = zmq_socket(context, ZMQ_REQ);
    if (!context || !requester) {
        fprintf(stderr, "Fehler beim Erstellen des ZMQ-Sockets.\n");
        zmq_close(requester);
        zmq_ctx_term(context);
        return 1;
    }

    for (int i = 2; i < argc; i++) {
        char endpoint[30];
        snprintf(endpoint, sizeof(endpoint), "tcp://localhost:%s", argv[i]);
        if (zmq_connect(requester, endpoint) != 0) {
            fprintf(stderr, "Fehler beim Verbinden mit Worker auf Port %s\n", argv[i]);
        }
    }

    char map_request[MAX_MSG_LEN];
    snprintf(map_request, sizeof(map_request), "map%.*s", (int)(sizeof(map_request) - 4), text);

    zmq_send(requester, map_request, strlen(map_request), 0);

    char map_result[MAX_MSG_LEN] = {0}; // Direkte Initialisierung auf 0
    int received_bytes = zmq_recv(requester, map_result, MAX_MSG_LEN - 1, 0);
    if (received_bytes < 0) {
        fprintf(stderr, "Fehler beim Empfang der Map-Response!\n");
        return 1;
    }
    map_result[received_bytes] = '\0'; // Sicherstellen, dass der String terminiert wird

    printf("Map-Result: %s\n", map_result);

    zmq_send(requester, "red", 3, 0);

    char reduce_result[MAX_MSG_LEN];
    memset(reduce_result, 0, MAX_MSG_LEN);
    zmq_recv(requester, reduce_result, MAX_MSG_LEN, 0);
    printf("Reduce-Result: %s\n", reduce_result);

    for (int i = 2; i < argc; i++) {
        zmq_send(requester, "rip", 3, 0);
        char rip_response[10];
        zmq_recv(requester, rip_response, 10, 0);
    }

    zmq_close(requester);
    zmq_ctx_term(context);
    return 0;
}
