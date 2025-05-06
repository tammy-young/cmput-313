#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/ttt.h"

typedef struct ServerP1 {
    int serverSocket;
} ServerP1;

ServerP1 *server_new();
void server_start(ServerP1 *s, int port);
void server_destroy(ServerP1 *s);
void server_runGame(ServerP1 *s, int clientSocket, TicTacToe *t);
bool server_handleServerMove(ServerP1 *s, TicTacToe *t, int clientSocket);
void server_sendGameState(ServerP1 *s, int clientSocket, TicTacToe *t,
                          char *cmd);

ServerP1 *server_new() {
    ServerP1 *server = (ServerP1 *)malloc(sizeof(ServerP1));
    if (!server) {
        exit(EXIT_FAILURE);
    }
    return server;
}

void server_start(ServerP1 *server, int port) {
    struct sockaddr_in serverAddr;

    // socket init
    server->serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (server->serverSocket == -1) {
        perror("error: failed to make socket");
        exit(EXIT_FAILURE);
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    // bind socket
    if (bind(server->serverSocket, (struct sockaddr *)&serverAddr,
             sizeof(serverAddr)) == -1) {
        perror("error: socket bind failed");
        close(server->serverSocket);
        exit(EXIT_FAILURE);
    }

    if (listen(server->serverSocket, 1) == -1) {
        perror("error: socket listen failed");
        close(server->serverSocket);
        exit(EXIT_FAILURE);
    }
}

void server_destroy(ServerP1 *server) {
    if (server->serverSocket != -1) {
        close(server->serverSocket);
    }
    free(server);
}

void server_runGame(ServerP1 *s, int clientSocket, TicTacToe *t) {
    char buffer[1024];
    bool gameRunning = true;

    while (gameRunning) {
        // get client move
        memset(buffer, 0, 1024);
        int bytesReceived = recv(clientSocket, buffer, 1024, 0);
        if (bytesReceived <= 0) {
            perror("Client disconnected or error receiving data");
            exit(EXIT_FAILURE);
        }

        char *token = strtok(buffer, " ");

        if (!strcmp(token, "TIMEOUT")) {
            printf("Client took too long to play. You won\n");
            ttt_drawBoard(t);
            exit(EXIT_SUCCESS);
        }

        token = strtok(NULL, " ");
        int move = atoi(token);

        // play client move
        if (!ttt_makeMove(t, move)) {
            printf("Invalid client move: %s\n", buffer);
            continue;
        }

        // check for resolution
        if (ttt_checkWin(t, 'O')) {
            printf("You won\n");
            ttt_drawBoard(t);
            server_sendGameState(s, clientSocket, t, buffer);
            gameRunning = false;
        } else if (ttt_checkWin(t, 'X')) {
            printf("You lost\n");
            ttt_drawBoard(t);
            server_sendGameState(s, clientSocket, t, buffer);
            gameRunning = false;
        } else if (ttt_isDraw(t)) {
            printf("You drew\n");
            ttt_drawBoard(t);
            server_sendGameState(s, clientSocket, t, buffer);
            gameRunning = false;
        } else {
            // handle server move
            ttt_drawBoard(t);
            gameRunning = server_handleServerMove(s, t, clientSocket);
        }
    }

    close(clientSocket);
    exit(EXIT_SUCCESS);
}

bool server_handleServerMove(ServerP1 *s, TicTacToe *t, int clientSocket) {
    char command[16];
    bool validCommand = false;

    while (!validCommand) {
        printf("> ");

        if (fgets(command, sizeof(command), stdin) == NULL) {
            printf("Error reading input\n");
            continue;
        }

        command[strcspn(command, "\n")] = '\0';

        char cmd[17];
        int n;
        char extra[1000];

        if (sscanf(command, "%16s %d %99s", cmd, &n, extra) != 2) {
            printf("Invalid command\n");
            continue;
        }

        if (ttt_makeMove(t, n)) {
            validCommand = true;
            if (ttt_checkWin(t, 'O')) {
                printf("You won\n");
                ttt_drawBoard(t);
                server_sendGameState(s, clientSocket, t, command);
                exit(EXIT_SUCCESS);
            } else if (ttt_checkWin(t, 'X')) {
                printf("You lost\n");
                ttt_drawBoard(t);
                server_sendGameState(s, clientSocket, t, command);
                exit(EXIT_SUCCESS);
            } else if (ttt_isDraw(t)) {
                printf("You drew\n");
                ttt_drawBoard(t);
                server_sendGameState(s, clientSocket, t, command);
                exit(EXIT_SUCCESS);
            }
        } else {
            printf("Must mark an unoccupied box on the board (1-9)\n");
        }
    }

    server_sendGameState(s, clientSocket, t, command);

    return true;
}

void server_sendGameState(ServerP1 *s, int clientSocket, TicTacToe *t,
                          char *cmd) {
    char gameState[1024];
    snprintf(gameState, 1024, "%s", cmd);
    gameState[1023] = '\0';
    send(clientSocket, gameState, strlen(gameState) + 1, 0);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    printf("Game started\n");

    ServerP1 *server = server_new();
    server_start(server, atoi(argv[1]));

    int clientSocket = accept(server->serverSocket, NULL, NULL);

    TicTacToe *t = ttt_new();

    server_runGame(server, clientSocket, t);

    return 0;
}
