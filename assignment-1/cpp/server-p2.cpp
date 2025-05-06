#include <string>
#include "ttt.h"
#include <pthread.h>

using namespace std;

struct GameRoom {
    int player1Socket;          
    int player2Socket;       
    int numActivePlayers;   
    TicTacToe game;            
    // This lock need to be acquired before accessing shared variables (e.g., player1Socket, player2Socket, numActivePlayers), and unlocked afterward
    pthread_mutex_t roomMutex;  
};

GameRoom rooms[5];

class ServerP2 {

public:
    ServerP2();
    void start();
    ~ServerP2();

private:
    const int serverSocket; 
    void handleNewClient(void* arg);
    bool createRoom(const string& roomId, int clientSocket);
    bool joinRoom(const string& roomId, int clientSocket);
    void runGame(int clientSocket, TicTacToe& game);
    void sendGameState(int clientSocket, const TicTacToe& game, const string& command = "");
    void handleDisconnect(const string& roomId, int clientSocket);
};

int main(int argc, char *argv[])
{    
    return 0;
}