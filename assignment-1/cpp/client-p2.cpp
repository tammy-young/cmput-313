#include <string>
#include <iostream>
#include <sstream>
#include "ttt.h"
using namespace std;

class ClientP2 {
public:
    ClientP2();
    void start();
    ~ClientP2();

private:
    int clientSocket;
    void connectToServer(int port);
    void handleRoomSelection();
    void runGame();
    void sendCommand(const string& command);
    string receiveServerResponse();

};

/**
 * Standard input parser for your command line program
 */

void input_parser() {
    char command[16]; 
    cout << "> ";
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
    else if (cmd == "JOIN") {
        cout << "valid command: JOIN " << num << endl;
        // TODO: implement how you handle the JOIN command
    }
    else {
        cout << "Invalid command" << endl;
    }
    return;
}

int main(int argc, char *argv[])
{   
    TicTacToe game = TicTacToe();
    // a read–eval–print loop for standard commands
    while (1){
        input_parser();
        // draw the game board as an example response 
        game.drawBoard();
    }
    return 0;
}
