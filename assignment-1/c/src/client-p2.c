#include <arpa/inet.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/ttt.h"

typedef struct ClientP2 {
    int clientSocket;
} ClientP2;

// You will need to implement these
ClientP2* client2_new();
void client2_start(ClientP2* c);
void client2_destroy(ClientP2* c);
void client2_connectToServer(ClientP2* c, int port, char* ip);
void client2_handleRoomSelection(ClientP2* c);
void client2_runGame(ClientP2* c, TicTacToe* t);
void client2_sendCommand(ClientP2* c, char* cmd);
char* client2_receiveServerResponse(ClientP2* c, TicTacToe* t);

ClientP2* client2_new() {
    ClientP2* c = (ClientP2*)malloc(sizeof(ClientP2));
    if (!c) {
        perror("Failed to allocate client");
        return NULL;
    }
    c->clientSocket = -1;
    return c;
}

void client2_destroy(ClientP2* c) {
    if (!c) return;
    if (c->clientSocket >= 0) {
        close(c->clientSocket);
    }
    free(c);
}

void client2_connectToServer(ClientP2* c, int port, char* ip) {
    if (!c) return;

    c->clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (c->clientSocket < 0) {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &serverAddr.sin_addr) <= 0) {
        perror("invalid ip addr");
        exit(EXIT_FAILURE);
    }

    if (connect(c->clientSocket, (struct sockaddr*)&serverAddr,
                sizeof(serverAddr)) < 0) {
        perror("couldn't connect to server");
        exit(EXIT_FAILURE);
    }
}

void client2_handleRoomSelection(ClientP2* c) {
    char roomCommand[16];
    int roomId;
    bool validRoom = false;

    while (!validRoom) {
        printf("> ");

        if (fgets(roomCommand, sizeof(roomCommand), stdin) == NULL) {
            printf("Failed to read input\n");
            continue;
        }

        if (sscanf(roomCommand, "JOIN %d", &roomId) != 1) {
            printf("Invalid command. Use JOIN <roomId>\n");
            continue;
        } else {
            printf("Joined room %d\n", roomId);
            client2_sendCommand(c, roomCommand);
            validRoom = true;
        }
    }
}

void client2_runGame(ClientP2* c, TicTacToe* t) {
    printf("Game started\n");

    while (true) {
        // wait for TURN or WAIT command
        printf("Waiting for server response...\n");
        char* response = client2_receiveServerResponse(c, t);
        printf("got response: %s\n", response);

        client2_sendCommand(c, "ACK");

        if (!strcmp(response, "TURN")) {
            while (1) {
                printf("> ");

                char command[16];
                if (fgets(command, sizeof(command), stdin) == NULL) {
                    printf("Failed to read input\n");
                    break;
                }

                int position;
                if (sscanf(command, "MARK %d", &position) == 1) {
                    client2_sendCommand(c, command);
                    ttt_makeMove(t, position);
                    break;  // got a valid move
                } else {
                    printf("Invalid command. Use MARK <position>\n");
                }
            }
        }

        if (ttt_checkWin(t, 'X')) {
            printf("You won\n");
            ttt_drawBoard(t);
            exit(EXIT_SUCCESS);
        } else if (ttt_checkWin(t, 'O')) {
            printf("You lost\n");
            ttt_drawBoard(t);
            exit(EXIT_SUCCESS);
        } else if (ttt_isDraw(t)) {
            printf("You drew\n");
            ttt_drawBoard(t);
            exit(EXIT_SUCCESS);
        }

        printf("\n");
    }
}

void client2_sendCommand(ClientP2* c, char* cmd) {
    if (!c || c->clientSocket < 0 || !cmd) return;

    if (send(c->clientSocket, cmd, strlen(cmd), 0) < 0) {
        perror("Failed to send command");
    }
}

char* client2_receiveServerResponse(ClientP2* c, TicTacToe* t) {
    if (!c || c->clientSocket < 0) return NULL;

    char buffer[512];
    memset(buffer, '\0', sizeof(buffer));

    int bytesReceived = recv(c->clientSocket, buffer, sizeof(buffer) - 1, 0);

    if (bytesReceived < 0) {
        perror("failed to get resp");
        return NULL;
    } else if (bytesReceived == 0) {
        printf("server closed connection\n");
        exit(EXIT_SUCCESS);
    }

    char* response = (char*)malloc(bytesReceived + 1);

    char cmd[17], extra[1000];
    int n;

    printf("received: %s\n", buffer);

    if (sscanf(buffer, "%16s %d %99s", cmd, &n, extra) == 2) {
        if (!strcmp(cmd, "MARK")) {
            printf("playing the move\n");
            ttt_makeMove(t, n);
            ttt_drawBoard(t);
        } else {
            printf("Unrecognized command\n");
        }
    } else {
        if (!strcmp(buffer, "WIN")) {
            printf("You won\n");
            client2_destroy(c);
            exit(EXIT_SUCCESS);
        } else if (!strcmp(buffer, "LOSE")) {
            printf("You lose\n");
            client2_destroy(c);
            exit(EXIT_SUCCESS);
        } else if (!strcmp(buffer, "DRAW")) {
            printf("You drew\n");
            client2_destroy(c);
            exit(EXIT_SUCCESS);
        } else if (!strcmp(buffer, "TURN")) {
            printf("Your turn\n");
        } else if (!strcmp(buffer, "WAIT")) {
            printf("Opponent's turn\n");
        } else {
            printf("Unrecognized command\n");
        }
    }

    strncpy(response, buffer, bytesReceived);
    response[bytesReceived] = '\0';
    return response;
}

void input_parser(ClientP2* c) {
    char command[16];

    if (fgets(command, sizeof(command), stdin) == NULL) {
        printf("Error reading input\n");
        return;
    }

    command[strcspn(command, "\n")] = '\0';

    char cmd[16];
    int n;

    if (sscanf(command, "%15s %d", cmd, &n) == 2) {
        if (!strcmp(cmd, "MARK")) {
            char markCmd[16];
            snprintf(markCmd, sizeof(markCmd), "MARK %d", n);
            client2_sendCommand(c, markCmd);
        } else if (!strcmp(cmd, "JOIN")) {
            char joinCmd[16];
            snprintf(joinCmd, sizeof(joinCmd), "JOIN %d", n);
            client2_sendCommand(c, joinCmd);
        } else {
            printf("Invalid command\n");
        }
    } else {
        printf("Invalid command\n");
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <ip> <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int port = atoi(argv[2]);
    if (port <= 0) {
        fprintf(stderr, "Invalid port number: %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    ClientP2* client = client2_new();
    TicTacToe* t = ttt_new();
    if (!client) {
        return EXIT_FAILURE;
    }

    client2_connectToServer(client, port, argv[1]);
    client2_handleRoomSelection(client);

    client2_runGame(client, t);

    client2_destroy(client);
    return EXIT_SUCCESS;
}
