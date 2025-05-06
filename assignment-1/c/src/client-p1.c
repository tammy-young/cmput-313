#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "../include/ttt.h"

typedef struct ClientP1 {
    int clientSocket;
    struct sockaddr_in serverAddr;
} ClientP1;

// You will need to implement these
ClientP1 *client_new(const char *serverIp, int port);
void client_start(ClientP1 *c);
void client_destroy(ClientP1 *c);
void client_sendCommand(ClientP1 *c, char *cmd);
char *client_receiveServerResponse(ClientP1 *c, TicTacToe *t);

ClientP1 *client_new(const char *serverIp, int port) {
    ClientP1 *c = (ClientP1 *)malloc(sizeof(ClientP1));
    if (!c) {
        perror("Failed to allocate ClientP1");
        exit(EXIT_FAILURE);
    }

    // create socket
    c->clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (c->clientSocket == -1) {
        perror("Socket creation failed");
        free(c);
        exit(EXIT_FAILURE);
    }

    memset(&c->serverAddr, 0, sizeof(c->serverAddr));
    c->serverAddr.sin_family = AF_INET;
    c->serverAddr.sin_port = htons(port);

    // Convert the IP address from text to binary form
    if (inet_pton(AF_INET, serverIp, &c->serverAddr.sin_addr) <= 0) {
        perror("Invalid server IP address");
        close(c->clientSocket);
        free(c);
        exit(EXIT_FAILURE);
    }

    return c;
}

void client_start(ClientP1 *c) {
    if (connect(c->clientSocket, (struct sockaddr *)&c->serverAddr,
                sizeof(c->serverAddr)) == -1) {
        perror("Connection to server failed");
        close(c->clientSocket);
        free(c);
        exit(EXIT_FAILURE);
    }
}

void client_destroy(ClientP1 *c) {
    if (c->clientSocket != -1) {
        close(c->clientSocket);
    }
    free(c);
}

void client_sendCommand(ClientP1 *c, char *cmd) {
    if (send(c->clientSocket, cmd, strlen(cmd), 0) == -1) {
        perror("Failed to send command to server");
    }
}

char *client_receiveServerResponse(ClientP1 *c, TicTacToe *t) {
    static char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    int bytesReceived = recv(c->clientSocket, buffer, sizeof(buffer) - 1, 0);

    if (bytesReceived <= 0) {
        perror("Failed to receive server response");
        exit(EXIT_FAILURE);
    }

    buffer[bytesReceived] = '\0';

    char cmd[17], extra[1000];
    int n;

    if (sscanf(buffer, "%16s %d %99s", cmd, &n, extra) == 2) {
        if (!strcmp(cmd, "MARK")) {
            ttt_makeMove(t, n);
            ttt_drawBoard(t);
        } else {
            printf("Unrecognized command\n");
        }
    } else {
        printf("Failed to parse server response.\n");
    }

    return buffer;
}

volatile bool timeout = false;
void handle_timeout(int sig) {
    if (sig == SIGALRM) {
        timeout = true;
    }
}

void input_parser(ClientP1 *client, TicTacToe *t) {
    char command[16];
    bool validCommand = false;
    alarm(30);
    timeout = false;

    struct sigaction sa;
    sa.sa_handler = handle_timeout;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGALRM, &sa, NULL);

    while (!validCommand) {

        printf("> ");

        char *line = fgets(command, sizeof(command), stdin);

        if (timeout) {
            printf("\nTime limit exceeded! You lose.\n");
            client_sendCommand(client, "TIMEOUT -1");
            ttt_drawBoard(t);
            exit(EXIT_SUCCESS);
        }

        if (line == NULL) {
            printf("Error reading input\n");
            continue;
        }

        command[strcspn(command, "\n")] = '\0';

        char cmd[17], extra[1000];
        int n;

        if (sscanf(command, "%16s %d %99s", cmd, &n, extra) != 2) {
            printf("Invalid command\n");
            continue;
        }

        if (!strcmp(cmd, "MARK")) {
            validCommand = true;
            alarm(0);
            if (ttt_makeMove(t, n)) {
                if (ttt_checkWin(t, 'X')) {
                    printf("You won\n");
                    ttt_drawBoard(t);
                    client_sendCommand(client, command);
                    exit(EXIT_SUCCESS);
                } else if (ttt_checkWin(t, 'O')) {
                    printf("You lost\n");
                    ttt_drawBoard(t);
                    client_sendCommand(client, command);
                    exit(EXIT_SUCCESS);
                } else if (ttt_isDraw(t)) {
                    printf("You drew\n");
                    ttt_drawBoard(t);
                    client_sendCommand(client, command);
                    exit(EXIT_SUCCESS);
                }
                client_sendCommand(client, command);
            } else {
                printf("Must mark an unoccupied box on the board (1-9)\n");
                validCommand = false;
                continue;
            }
        } else {
            printf("Unrecognized command\n");
            continue;
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("Usage: %s <ip> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    TicTacToe *t = ttt_new();
    ClientP1 *client = client_new(argv[1], atoi(argv[2]));
    client_start(client);
    bool gameRunning = true;

    printf("You go first\n");

    while (gameRunning) {
        input_parser(client, t);

        char *command = client_receiveServerResponse(client, t);

        if (ttt_checkWin(t, 'X')) {
            gameRunning = false;
            printf("You won\n");
            ttt_drawBoard(t);
            client_sendCommand(client, command);
            exit(EXIT_SUCCESS);
        } else if (ttt_checkWin(t, 'O')) {
            gameRunning = false;
            printf("You lost\n");
            ttt_drawBoard(t);
            client_sendCommand(client, command);
            exit(EXIT_SUCCESS);
        } else if (ttt_isDraw(t)) {
            gameRunning = false;
            printf("You drew\n");
            ttt_drawBoard(t);
            client_sendCommand(client, command);
            exit(EXIT_SUCCESS);
        }
    }
    exit(EXIT_SUCCESS);
}
