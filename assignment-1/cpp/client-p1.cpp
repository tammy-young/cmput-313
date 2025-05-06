#include <string>
#include <iostream>
#include <sstream>
#include "ttt.h"
using namespace std;

class ClientP1 {
public:
    ClientP1();
    void start();
    ~ClientP1();

private:
    int clientSocket;
    void sendCommand(const string& command);
    string receiveServerResponse();
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
    TicTacToe game = TicTacToe();
    // a read–eval–print loop for standard commands
    while (1){
        cout << "> ";
        input_parser();
        // draw the game board as an example response 
        game.drawBoard();
    }
    return 0;
}
