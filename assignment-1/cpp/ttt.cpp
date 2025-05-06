// Modified from: https://www.geeksforgeeks.org/tic-tac-toe-game-in-cpp/
#include <iostream>
#include "ttt.h"
using namespace std;

void TicTacToe::convertMove(int move, int& row, int& col) {
    row = (move - 1) / 3;
    col = (move - 1) % 3;
}

bool TicTacToe::checkWin(char player) {
    // Check rows and columns
    for (int i = 0; i < 3; i++) {
        if (board[i][0] == player && board[i][1] == player
            && board[i][2] == player)
            return true;
        if (board[0][i] == player && board[1][i] == player
            && board[2][i] == player)
            return true;
    }
    // Check diagonals
    if (board[0][0] == player && board[1][1] == player
        && board[2][2] == player)
        return true;
    if (board[0][2] == player && board[1][1] == player
        && board[2][0] == player)
        return true;
    return false;
}

TicTacToe::TicTacToe() {
    reset();
}

void TicTacToe::reset() {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            board[i][j] = ' ';
        }
    }
    currentPlayer = 'X';
    turnCount = 0;
}

void TicTacToe::drawBoard() {
    cout << "-------------\n";
    for (int i = 0; i < 3; i++) {
        cout << "| ";
        for (int j = 0; j < 3; j++) {
            char displayChar = board[i][j];
            if (displayChar == ' ')
                displayChar = '0' + (i * 3 + j + 1);
            cout << displayChar << " | ";
        }
        cout << "\n-------------\n";
    }
}

bool TicTacToe::makeMove(int move) {
    int row, col;
    
    if (move < 1 || move > 9) {
        return false;
    }

    convertMove(move, row, col);
    
    if (board[row][col] != ' ') {
        return false;
    }

    board[row][col] = currentPlayer;
    turnCount++;
    return true;
}

bool TicTacToe::hasWinner() {
    return checkWin(currentPlayer);
}

bool TicTacToe::isDraw() {
    return turnCount == 9 && !hasWinner();
}

void TicTacToe::switchPlayer() {
    currentPlayer = (currentPlayer == 'X') ? 'O' : 'X';
}

char TicTacToe::getCurrentPlayer() {
    return currentPlayer;
}

char TicTacToe::getBoardCell(int row, int col) const {
    return board[row][col];
}
