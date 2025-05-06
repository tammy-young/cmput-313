#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../include/ttt.h"

typedef struct GameRoom {
    int player1Socket;
    int player2Socket;
    int numActivePlayers;
    bool gameRunning;
    TicTacToe* t;
    pthread_mutex_t roomMutex;
} GameRoom;

typedef struct ServerP2 {
    int serverSocket;
    GameRoom* rooms[5];
} ServerP2;

ServerP2* server = NULL;

ServerP2* server2_new();
void server2_start(ServerP2* s, int port);
void server2_destroy(ServerP2* s);
void* server2_handleNewClient(void* arg);
bool server2_createRoom(ServerP2* s, char* roomId, int clientSocket);
bool server2_joinRoom(ServerP2* s, char* roomId, int clientSocket);
void server2_runGame(ServerP2* s, void* arg);
void server2_sendGameState(ServerP2* s, int clientSocket, TicTacToe* t,
                           char* cmd);
void server2_handleDisconnect(ServerP2* s, char* roomId, int clientSocket);

ServerP2* server2_new() {
    ServerP2* s = (ServerP2*)malloc(sizeof(ServerP2));
    if (!s) return NULL;

    for (int i = 0; i < 5; i++) {
        s->rooms[i] = (GameRoom*)malloc(sizeof(GameRoom));
        if (!s->rooms[i]) continue;

        s->rooms[i]->player1Socket = -1;
        s->rooms[i]->player2Socket = -1;
        s->rooms[i]->numActivePlayers = 0;
        s->rooms[i]->t = NULL;
        pthread_mutex_init(&s->rooms[i]->roomMutex, NULL);
    }
    return s;
}

void server2_start(ServerP2* s, int port) {
    struct sockaddr_in serverAddr;
    s->serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (s->serverSocket == -1) {
        perror("error: failed to make socket");
        exit(EXIT_FAILURE);
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(s->serverSocket, (struct sockaddr*)&serverAddr,
             sizeof(serverAddr)) == -1) {
        perror("error: socket bind failed");
        close(s->serverSocket);
        exit(EXIT_FAILURE);
    }

    if (listen(s->serverSocket, 10) == -1) {
        perror("Listen failed");
        close(s->serverSocket);
        exit(EXIT_FAILURE);
    }

    // handle clients
    while (1) {
        int clientSocket = accept(server->serverSocket, NULL, NULL);
        if (clientSocket < 0) {
            perror("accepting client failed");
            exit(EXIT_FAILURE);
        }

        // new client -> new thread
        int* clientSocketPtr = (int*)malloc(sizeof(int));
        *clientSocketPtr = clientSocket;
        pthread_t thread;
        pthread_create(&thread, NULL, server2_handleNewClient, clientSocketPtr);
        pthread_detach(thread);
    }
}

void* server2_handleNewClient(void* arg) {
    int clientSocket = *(int*)arg;
    ServerP2* s = server;

    if (arg == NULL) {
        perror("Received NULL argument");
        exit(EXIT_FAILURE);
    }

    char buffer[256];
    memset(buffer, '\0', sizeof(buffer));

    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

    if (bytesReceived <= 0) {
        perror("Failed to receive command");
        close(clientSocket);
        exit(EXIT_FAILURE);
    }

    // get room number
    if (strncmp(buffer, "JOIN ", 5) == 0) {
        char* roomId = buffer + 5;
        if (!server2_joinRoom(s, roomId, clientSocket)) {
            printf("Failed to join room %s\n", roomId);
            close(clientSocket);
        }
    }

    return 0;
}

bool server2_createRoom(ServerP2* s, char* roomId, int clientSocket) {
    int roomIndex = atoi(roomId);
    if (roomIndex < 0 || roomIndex >= 5) return false;

    GameRoom* room = s->rooms[roomIndex];
    pthread_mutex_lock(&room->roomMutex);

    printf("Client created room %d\n", roomIndex + 1);
    room->player1Socket = clientSocket;
    room->numActivePlayers = 1;
    pthread_mutex_unlock(&room->roomMutex);
    return true;
}

bool server2_joinRoom(ServerP2* s, char* roomId, int clientSocket) {
    int roomIndex = atoi(roomId);
    if (roomIndex < 0 || roomIndex >= 5) return false;

    GameRoom* room = s->rooms[roomIndex];
    pthread_mutex_lock(&room->roomMutex);

    if (room->numActivePlayers == 0) {
        pthread_mutex_unlock(&room->roomMutex);
        server2_createRoom(s, roomId, clientSocket);
        return true;
    } else if (room->numActivePlayers == 1) {
        // only start the game when there's two players
        printf("client joined room %d\n", roomIndex + 1);
        room->player2Socket = clientSocket;
        room->numActivePlayers = 2;

        if (!room->gameRunning) {
            room->gameRunning = true;
            pthread_mutex_unlock(&room->roomMutex);
            server2_runGame(s, room);
            return true;
        }
    }

    pthread_mutex_unlock(&room->roomMutex);
    printf("Room is full %s\n", roomId);
    return false;  // Room is full
}

void server2_runGame(ServerP2* s, void* arg) {
    GameRoom* room = (GameRoom*)arg;
    room->t = (TicTacToe*)malloc(sizeof(TicTacToe));
    memset(room->t->board, ' ', sizeof(room->t->board));
    room->t->currentPlayer = 'X';

    printf("Game started\n");

    while (1) {
        int currentPlayerSocket = (room->t->currentPlayer == 'X')
                                      ? room->player1Socket
                                      : room->player2Socket;
        int opponentSocket = (room->t->currentPlayer == 'X')
                                 ? room->player2Socket
                                 : room->player1Socket;

        server2_sendGameState(s, currentPlayerSocket, room->t, "TURN");
        server2_sendGameState(s, opponentSocket, room->t, "WAIT");

        printf("player %c's turn\n", room->t->currentPlayer);

        char ackBuffer[16];
        memset(ackBuffer, '\0', sizeof(ackBuffer));
        recv(currentPlayerSocket, ackBuffer, sizeof(ackBuffer) - 1, 0);
        printf("received ACK from player %c\n", room->t->currentPlayer);

        // get move
        char buffer[256];
        memset(buffer, '\0', sizeof(buffer));
        int bytesReceived =
            recv(currentPlayerSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) {
            perror("connection with client closed");
            exit(EXIT_SUCCESS);
        }
        buffer[bytesReceived] = '\0';

        if (strncmp(buffer, "MARK ", 5) == 0) {
            printf("received command: %s\n", buffer);
            int move = atoi(buffer + 5);

            ttt_makeMove(room->t, move);
            server2_sendGameState(s, opponentSocket, room->t, buffer);
            server2_sendGameState(s, currentPlayerSocket, room->t, buffer);

            if (ttt_checkWin(room->t, 'X')) {
                printf("Player 1 won\n");
                server2_sendGameState(s, room->player1Socket, room->t, "WIN");
                server2_sendGameState(s, room->player2Socket, room->t, "LOSE");
                break;
            } else if (ttt_checkWin(room->t, 'O')) {
                printf("Player 2 won\n");
                server2_sendGameState(s, room->player1Socket, room->t, "LOSE");
                server2_sendGameState(s, room->player2Socket, room->t, "WIN");
                break;
            } else if (ttt_isDraw(room->t)) {
                printf("Game is a draw\n");
                server2_sendGameState(s, room->player1Socket, room->t, "DRAW");
                server2_sendGameState(s, room->player2Socket, room->t, "DRAW");
                break;
            }
        }

        printf("\n");
    }

    // Clean up after the game
    free(room->t);
    room->t = NULL;
    close(room->player1Socket);
    close(room->player2Socket);
    pthread_mutex_lock(&room->roomMutex);
    room->gameRunning = false;
    room->player1Socket = -1;
    room->player2Socket = -1;
    room->numActivePlayers = 0;
    pthread_mutex_unlock(&room->roomMutex);
    server2_destroy(s);
    exit(EXIT_SUCCESS);
}

void server2_destroy(ServerP2* s) {
    if (!s) return;

    if (s->serverSocket >= 0) {
        close(s->serverSocket);
    }

    for (int i = 0; i < 5; i++) {
        GameRoom* room = s->rooms[i];
        if (room) {
            pthread_mutex_destroy(&room->roomMutex);
            if (room->t) {
                free(room->t);
            }
            free(room);
        }
    }

    free(s);
}

void server2_sendGameState(ServerP2* s, int clientSocket, TicTacToe* t,
                           char* cmd) {
    char gameState[1024];
    snprintf(gameState, 1024, "%s", cmd);
    gameState[1023] = '\0';
    send(clientSocket, gameState, strlen(gameState) + 1, 0)
}

void handle_exit(int sig) {
    if (server) {
        printf("\nshutting down server\n");
        server2_destroy(server);
        exit(0);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int port = atoi(argv[1]);
    if (port <= 0) {
        printf("Invalid port number: %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    signal(SIGINT, handle_exit);

    server = server2_new();
    server2_start(server, port);

    server2_destroy(server);

    return EXIT_SUCCESS;
}
