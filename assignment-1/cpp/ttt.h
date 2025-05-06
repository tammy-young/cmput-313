// Modified from: https://www.geeksforgeeks.org/tic-tac-toe-game-in-cpp/
#ifndef TTT_H
#define TTT_H

class TicTacToe {
private:
    char board[3][3];
    char currentPlayer;
    int turnCount;
    
    void convertMove(int move, int& row, int& col);
    bool checkWin(char player);

public:
    TicTacToe();
    void reset();
    void drawBoard();
    bool makeMove(int move);
    bool hasWinner();
    bool isDraw();
    void switchPlayer();
    char getCurrentPlayer();
    char getBoardCell(int row, int col) const;  
};

#endif 
