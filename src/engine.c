#include <stdio.h>
#include <stdlib.h>
#include "include/Headers/move.h"
// int choose_move(char *fen, char *moves, int timeout) {
//     if (fen && moves) printf("")
//     else return 0;
// }





//This function checks if a king is in "check" possition.
//"board[8][8]" is our chess board (the current possition of the game).
//"isWhite" is given from FEN ("w" or "b") that indicates which player is playing. 
//In main if FEN returns "w" then isWhite will bwcome '1', and if it returns "b" then isWhite will become '0'.
int is_king_in_check(char board[8][8], int isWhite) {
    // Find the position of the king
    int king_row, king_col;
    char king_symbol;
    
    // Determine the king's symbol based on the player
    if (isWhite) { //meaning white is playing
        king_symbol = 'K';  // 'K' for white king
    } else { //meaning black is playing
        king_symbol = 'k';  // 'k' for black king
    }

    // Locate the king on the board
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            if (board[i][j] == king_symbol) {
                king_row = i;
                king_col = j;
                break; // King found, exit loop
            }
        }
    }

    // Check for opponent pieces based on the player
    char opponent_pawn, opponent_knight, opponent_bishop, opponent_rook, opponent_queen, opponent_king;

    if (isWhite) { //if white is playing, (obviously) oppenent pieces are black (meaning small letters)
        opponent_pawn = 'p';
        opponent_knight = 'n';
        opponent_bishop = 'b';
        opponent_rook = 'r';
        opponent_queen = 'q';
        opponent_king = 'k';
    } else { //if black is playing, (obviously) oppenent pieces are white (meaning capital letters)
        opponent_pawn = 'P';
        opponent_knight = 'N';
        opponent_bishop = 'B';
        opponent_rook = 'R';
        opponent_queen = 'Q';
        opponent_king = 'K';
    }

    //for every enemy piece i have to see if it attacks the king, i do this by checking if the king's possition on the board can be captured by any other peace 

    // Check for pawn attacks
    if (isWhite) {
        // White king: check from black pawns
        if ((king_row + 1 < 8 && king_col - 1 >= 0 && board[king_row + 1][king_col - 1] == opponent_pawn) ||
            (king_row + 1 < 8 && king_col + 1 < 8 && board[king_row + 1][king_col + 1] == opponent_pawn)) {
            return 1; // King is in check
        }
    } else {
        // Black king: check from white pawns
        if ((king_row - 1 >= 0 && king_col - 1 >= 0 && board[king_row - 1][king_col - 1] == opponent_pawn) ||
            (king_row - 1 >= 0 && king_col + 1 < 8 && board[king_row - 1][king_col + 1] == opponent_pawn)) {
            return 1; // King is in check
        }
    }

    // Check for knight attacks
    int knight_moves[8][2] = {
        {-2, -1}, {-2, 1}, {2, -1}, {2, 1},
        {-1, -2}, {1, -2}, {-1, 2}, {1, 2}
    };
    for (int i = 0; i < 8; i++) {
        int new_row = king_row + knight_moves[i][0];
        int new_col = king_col + knight_moves[i][1];
        
        if (new_row >= 0 && new_row < 8 && new_col >= 0 && new_col < 8) {
            if (isWhite) {
                if (board[new_row][new_col] == opponent_knight) {
                    return 1; // King is in check
                }
            } else {
                if (board[new_row][new_col] == 'N') {
                    return 1; // King is in check
                }
            }
        }
    }

    // Check for linear attacks from rooks, bishops, or queens
    char* directions[] = {
        "N", "S", "E", "W",  // Vertical and horizontal directions
        "NE", "NW", "SE", "SW"  // Diagonal directions
    };

    // Check for all directions (up, down, left, right, diagonals)
    for (int d = 0; d < 8; d++) {
        int dr = 0, dc = 0;

        if (d < 4) { // Vertical and horizontal
            if (d == 0) {
                dr = -1; // North
                dc = 0;
            } else if (d == 1) {
                dr = 1; // South
                dc = 0;
            } else if (d == 2) {
                dr = 0;
                dc = 1; // East
            } else if (d == 3) {
                dr = 0;
                dc = -1; // West
            }
        } else { // Diagonal
            if (d == 4) {
                dr = -1; // Northeast
                dc = -1;
            } else if (d == 5) {
                dr = -1; // Northwest
                dc = 1;
            } else if (d == 6) {
                dr = 1; // Southeast
                dc = -1;
            } else if (d == 7) {
                dr = 1; // Southwest
                dc = 1;
            }
        }

        for (int i = 1; i < 8; i++) {
            int new_row = king_row + dr * i;
            int new_col = king_col + dc * i;

            if (new_row < 0 || new_row >= 8 || new_col < 0 || new_col >= 8) {
                break; // Out of bounds
            }

            char piece = board[new_row][new_col];
            if (piece != ' ') { // There's a piece in the way
                if (isWhite) {
                    // White king: check from rook or queen
                    if (piece == opponent_rook || piece == opponent_queen) {
                        return 1; // King is in check
                    }
                    // White king: check from bishop or queen
                    if (piece == opponent_bishop || piece == opponent_queen) {
                        return 1; // King is in check
                    }
                } else {
                    // Black king: check from rook or queen
                    if (piece == 'R' || piece == 'Q') {
                        return 1; // King is in check
                    }
                    // Black king: check from bishop or queen
                    if (piece == 'B' || piece == 'Q') {
                        return 1; // King is in check
                    }
                }
                break; // Stop checking in this direction
            }
        }
    }
    return 0; // King is not in check
}











int main(int argc, char *argv[]) {
    // if (argc != 4) exit(EXIT_FAILURE);
    printf("%d\n", move(1));
    // choose_move(argv[1], argv[2], atoi(argv[3]));
    exit(EXIT_SUCCESS);
}
