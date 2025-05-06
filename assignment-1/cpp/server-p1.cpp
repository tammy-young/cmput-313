#include <string>
#include "ttt.h"
#include <iostream>
#include <sstream>

using namespace std;

class ServerP1 {
public:
    ServerP1();
    void start();
    ~ServerP1();

private:
    const int serverSocket;
    void runGame(int clientSocket, TicTacToe& game);
    bool handleServerMove(TicTacToe& game, int clientSocket);
    void sendGameState(int clientSocket, TicTacToe& game, const string& command = "");
};

/**
 * Standard input parser for your command line program
 */

void input_parser() {
    char command[16]; 
    cin.getline(command, 16);
    istringstream stream(command);
    string cmd;
    int num;
    if (!(stream >> cmd >> num) || !stream.eof()) {
        cout << "Invalid command" << endl;
        return;
    }
    if (cmd == "MARK") {
        cout << "valid command: MARK " << num << endl;
        // TODO: implement how you handle the MARK command
    }
    else {
        cout << "Invalid command" << endl;
    }
    return;
}

int main(int argc, char *argv[])
{   
    // a read–eval–print loop for standard commands
    while (1){
        cout << "> ";
        input_parser();
    }
    return 0;
}

