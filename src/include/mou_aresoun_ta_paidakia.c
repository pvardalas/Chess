#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Structure to store a move (up to 5 characters plus the null terminator).
// For example, a pawn move might be "c5" and a knight move "Nc3".
typedef struct {
    char move[6];
} Move;

// Convert board coordinates to algebraic notation.
// Our board uses row 0 for rank 8 and row 7 for rank 1.
void coord_to_square(int row, int col, char square[3]) {
    square[0] = 'a' + col;       // file letter
    square[1] = '8' - row;       // rank digit
    square[2] = '\0';
}

// Adds a move string to the moves array.
void add_move(Move moves[], int *moveCount, const char *moveStr) {
    strcpy(moves[*moveCount].move, moveStr);
    (*moveCount)++;
}

// Check if the given board coordinates are within bounds.
int is_valid(int row, int col) {
    return (row >= 0 && row < 8 && col >= 0 && col < 8);
}

// Helper functions to check piece color.
int is_white(char piece) {
    return (piece >= 'A' && piece <= 'Z');
}

int is_black(char piece) {
    return (piece >= 'a' && piece <= 'z');
}

// Add a move using the piece type, its starting square, destination square,
// and whether the move is a capture (for informational purposes).
// (For pawns we output only the destination square.)
void add_move_with_piece(char piece, int startRow, int startCol, int destRow, int destCol, int isCapture, Move moves[], int *moveCount) {
    char square[3];
    coord_to_square(destRow, destCol, square);
    char moveStr[6];
     
    if (piece == 'P' || piece == 'p') {
        // For pawns, we simply output the destination square.
        // (You might choose to indicate captures with an 'x', e.g. "dxc5", but the sample uses just "c5".)
        sprintf(moveStr, "%s", square);
    } else {
        // For other pieces, prefix with an uppercase letter.
        char prefix = toupper(piece);
        sprintf(moveStr, "%c%s", prefix, square);
    }
    add_move(moves, moveCount, moveStr);
}

// Generates every legal move (by movement rules only) for all pieces of the side to move.
// This version includes pawn capture moves.
void generate_moves(char board[8][8], int isWhiteTurn, Move moves[], int *moveCount) {
    *moveCount = 0;
    int row, col;
    
    for (row = 0; row < 8; row++) {
        for (col = 0; col < 8; col++) {
            char piece = board[row][col];
            if (piece == ' ') continue;
            if (isWhiteTurn && !is_white(piece)) continue;
            if (!isWhiteTurn && !is_black(piece)) continue;
            
            // Pawn moves (white pawn is 'P', black pawn is 'p')
            if (piece == 'P' || piece == 'p') {
                int direction = (piece == 'P') ? -1 : 1; // white pawns move up (decreasing row)
                int startRank = (piece == 'P') ? 6 : 1;
                int newRow = row + direction;
                
                // Forward move (only if the square is empty)
                if (is_valid(newRow, col) && board[newRow][col] == ' ') {
                    add_move_with_piece(piece, row, col, newRow, col, 0, moves, moveCount);
                    
                    // Double move from starting rank
                    if (row == startRank) {
                        int twoStep = row + 2 * direction;
                        if (is_valid(twoStep, col) && board[twoStep][col] == ' ' && board[newRow][col] == ' ') {
                            add_move_with_piece(piece, row, col, twoStep, col, 0, moves, moveCount);
                        }
                    }
                }
                // Capture moves: diagonal left and right
                for (int dc = -1; dc <= 1; dc += 2) {
                    int newCol = col + dc;
                    if (is_valid(newRow, newCol)) {
                        char target = board[newRow][newCol];
                        if (target != ' ' && ((isWhiteTurn && is_black(target)) || (!isWhiteTurn && is_white(target)))) {
                            add_move_with_piece(piece, row, col, newRow, newCol, 1, moves, moveCount);
                        }
                    }
                }
            }
            // Knight moves
            else if (toupper(piece) == 'N') {
                int knightOffsets[8][2] = {
                    {-2, -1}, {-2, 1}, {2, -1}, {2, 1},
                    {-1, -2}, {-1, 2}, {1, -2}, {1, 2}
                };
                for (int k = 0; k < 8; k++) {
                    int newRow = row + knightOffsets[k][0];
                    int newCol = col + knightOffsets[k][1];
                    if (is_valid(newRow, newCol)) {
                        char target = board[newRow][newCol];
                        if (target == ' ' || (isWhiteTurn && is_black(target)) || (!isWhiteTurn && is_white(target))) {
                            int isCapture = (target != ' ') ? 1 : 0;
                            add_move_with_piece(piece, row, col, newRow, newCol, isCapture, moves, moveCount);
                        }
                    }
                }
            }
            // Bishop moves (diagonals)
            else if (toupper(piece) == 'B') {
                int bishopDirs[4][2] = { {-1,-1}, {-1,1}, {1,-1}, {1,1} };
                for (int d = 0; d < 4; d++) {
                    for (int step = 1; step < 8; step++) {
                        int newRow = row + bishopDirs[d][0] * step;
                        int newCol = col + bishopDirs[d][1] * step;
                        if (!is_valid(newRow, newCol)) break;
                        char target = board[newRow][newCol];
                        if (target == ' ') {
                            add_move_with_piece(piece, row, col, newRow, newCol, 0, moves, moveCount);
                        } else {
                            if ((isWhiteTurn && is_black(target)) || (!isWhiteTurn && is_white(target))) {
                                add_move_with_piece(piece, row, col, newRow, newCol, 1, moves, moveCount);
                            }
                            break;
                        }
                    }
                }
            }
            // Rook moves (vertical and horizontal)
            else if (toupper(piece) == 'R') {
                int rookDirs[4][2] = { {-1,0}, {1,0}, {0,-1}, {0,1} };
                for (int d = 0; d < 4; d++) {
                    for (int step = 1; step < 8; step++) {
                        int newRow = row + rookDirs[d][0] * step;
                        int newCol = col + rookDirs[d][1] * step;
                        if (!is_valid(newRow, newCol)) break;
                        char target = board[newRow][newCol];
                        if (target == ' ') {
                            add_move_with_piece(piece, row, col, newRow, newCol, 0, moves, moveCount);
                        } else {
                            if ((isWhiteTurn && is_black(target)) || (!isWhiteTurn && is_white(target))) {
                                add_move_with_piece(piece, row, col, newRow, newCol, 1, moves, moveCount);
                            }
                            break;
                        }
                    }
                }
            }
            // Queen moves (combines rook and bishop)
            else if (toupper(piece) == 'Q') {
                int queenDirs[8][2] = {
                    {-1,0}, {1,0}, {0,-1}, {0,1},
                    {-1,-1}, {-1,1}, {1,-1}, {1,1}
                };
                for (int d = 0; d < 8; d++) {
                    for (int step = 1; step < 8; step++) {
                        int newRow = row + queenDirs[d][0] * step;
                        int newCol = col + queenDirs[d][1] * step;
                        if (!is_valid(newRow, newCol)) break;
                        char target = board[newRow][newCol];
                        if (target == ' ') {
                            add_move_with_piece(piece, row, col, newRow, newCol, 0, moves, moveCount);
                        } else {
                            if ((isWhiteTurn && is_black(target)) || (!isWhiteTurn && is_white(target))) {
                                add_move_with_piece(piece, row, col, newRow, newCol, 1, moves, moveCount);
                            }
                            break;
                        }
                    }
                }
            }
            // King moves (one square in any direction)
            else if (toupper(piece) == 'K') {
                int kingDirs[8][2] = {
                    {-1,0}, {1,0}, {0,-1}, {0,1},
                    {-1,-1}, {-1,1}, {1,-1}, {1,1}
                };
                for (int d = 0; d < 8; d++) {
                    int newRow = row + kingDirs[d][0];
                    int newCol = col + kingDirs[d][1];
                    if (is_valid(newRow, newCol)) {
                        char target = board[newRow][newCol];
                        if (target == ' ' || (isWhiteTurn && is_black(target)) || (!isWhiteTurn && is_white(target))) {
                            int isCapture = (target != ' ') ? 1 : 0;
                            add_move_with_piece(piece, row, col, newRow, newCol, isCapture, moves, moveCount);
                        }
                    }
                }
            }
        }
    }
}

int main() {
    // Example board position.
    // Rows: 0 -> rank 8, 7 -> rank 1.
    // Files: 0 -> A, 7 -> H.
    char board[8][8] = {
        /*8*/{'r','n','b','q','k','b','n','r'},
        /*7*/{'p','p','p','p','p','p','p','p'},
        /*6*/{' ',' ',' ',' ',' ',' ',' ',' '},
        /*5*/{' ',' ',' ',' ',' ',' ',' ',' '},
        /*4*/{' ',' ',' ',' ',' ',' ',' ',' '},
        /*3*/{' ',' ',' ',' ',' ',' ',' ',' '},
        /*2*/{'P','P','P','P','P','P','P','P'},
        /*1*/{'R','N','B','Q','K','B','N','R'}
        /*     A   B   C   D   E   F   G   H */
    };
    
    Move moves[256];
    int moveCount;
    
    // Generate legal moves for White.
    generate_moves(board, 1, moves, &moveCount);
    
    // Print the moves separated by commas.
    for (int i = 0; i < moveCount; i++) {
        printf("%s", moves[i].move);
        if (i < moveCount - 1)
            printf(",");
    }
    printf("\n");
    
    return 0;
}
