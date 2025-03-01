#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <stdint.h>
#include "Headers/fen.h"
#include "Headers/boardmap.h"
#define BOARD_SIZE 8
/*
Assumed FEN structure:
typedef struct {
    char board[8][8];   // Board squares; empty squares are stored as ' '.
    char turn;          // 'w' or 'b'
    char castling[5];   // Castling rights (e.g., "KQkq")
    char enPassant[5];  // En passant target square (e.g., "e3" or "-" if none)
    int halfMoveClock;  // Half–move clock (for the 50–move rule)
    int fullMoveNum;    // Full move number
} FEN;
*/

// ===== FEN PARSING AND GENERATION =====

// Fill the board with spaces for empty squares.
void parse_fen(const char *fen, FEN *position) {
    memset(position, 0, sizeof(FEN));
    for (int r = 0; r < BOARD_SIZE; r++) {
        for (int c = 0; c < BOARD_SIZE; c++) {
            position->board[r][c] = ' ';
        }
    }
    
    char *fenCopy = strdup(fen);
    char *token = strtok(fenCopy, " ");

    // Parse board layout.
    int row = 0, col = 0;
    for (int i = 0; token[i] != '\0'; i++) {
        if (token[i] == '/') {
            row++;
            col = 0;
        } else if (isdigit((unsigned char)token[i])) {
            int emptySquares = token[i] - '0';
            for (int j = 0; j < emptySquares; j++) {
                position->board[row][col++] = ' ';
            }
        } else {
            position->board[row][col++] = token[i];
        }
    }
    // Turn
    token = strtok(NULL, " ");
    position->turn = token[0];
    // Castling rights
    token = strtok(NULL, " ");
    strcpy(position->castling, token);
    
    // En passant target
    token = strtok(NULL, " ");
    strcpy(position->enPassant, token);
    // Half–move clock
    token = strtok(NULL, " ");
    position->halfMoveClock = atoi(token);
    // Full move number
    token = strtok(NULL, " ");
    if (token != NULL)
    position->fullMoveNum = atoi(token);
    printf("test2\n");

    free(fenCopy);
}

void generate_fen(FEN *position, char *fenBuffer) {
    char temp[100];
    int index = 0;
    // Board
    for (int row = 0; row < BOARD_SIZE; row++) {
        int emptyCount = 0;
        for (int col = 0; col < BOARD_SIZE; col++) {
            if (position->board[row][col] == ' ') {
                emptyCount++;
            } else {
                if (emptyCount > 0) {
                    temp[index++] = '0' + emptyCount;
                    emptyCount = 0;
                }
                temp[index++] = position->board[row][col];
            }
        }
        if (emptyCount > 0)
            temp[index++] = '0' + emptyCount;
        if (row < BOARD_SIZE - 1)
            temp[index++] = '/';
    }
    // Turn
    temp[index++] = ' ';
    temp[index++] = position->turn;
    // Castling rights
    temp[index++] = ' ';
    strcpy(temp + index, position->castling);
    index += strlen(position->castling);
    // En passant target
    temp[index++] = ' ';
    strcpy(temp + index, position->enPassant);
    index += strlen(position->enPassant);
    // Half-move clock
    sprintf(temp + index, " %d", position->halfMoveClock);
    index += strlen(temp + index);
    // Full move number
    sprintf(temp + index, " %d", position->fullMoveNum);
    index += strlen(temp + index);
    strcpy(fenBuffer, temp);
}

// ===== HELPER FUNCTIONS =====

// Convert board coordinates to algebraic square (e.g. row=7, col=0 -> "a1")
void coord_to_square(int row, int col, char *square) {
    square[0] = 'a' + col;
    square[1] = '8' - row;
    square[2] = '\0';
}

// Compare a given row/col to the en passant square (stored as a string in position->enPassant).
int is_enpassant_square(FEN *pos, int row, int col) {
    char sq[3];
    coord_to_square(row, col, sq);
    return (strcmp(sq, pos->enPassant) == 0);
}

// ===== MOVE VALIDATION FUNCTIONS =====

// Pawn move validation, including en passant and promotion (promotion is handled in apply_move).
int is_valid_move_pawn(FEN *pos, int sr, int sc, int dr, int dc) {
    int dRow = dr - sr;
    int dCol = dc - sc;
    if (pos->turn == 'w') {
        // Single forward move.
        if (dCol == 0 && dRow == -1 && pos->board[dr][dc] == ' ')
            return 1;
        // Double move from starting rank.
        if (dCol == 0 && sr == 6 && dRow == -2 && 
            pos->board[sr-1][sc]==' ' && pos->board[dr][dc]==' ')
            return 1;
        // Diagonal capture.
        if (abs(dCol)==1 && dRow==-1) {
            // Normal capture.
            if (pos->board[dr][dc] != ' ' && islower(pos->board[dr][dc]))
                return 1;
            // En passant capture.
            if (pos->board[dr][dc] == ' ' && is_enpassant_square(pos, dr, dc))
                return 1;
        }
    } else { // Black pawn.
        if (dCol == 0 && dRow == 1 && pos->board[dr][dc]==' ')
            return 1;
        if (dCol == 0 && sr==1 && dRow==2 &&
            pos->board[sr+1][sc]==' ' && pos->board[dr][dc]==' ')
            return 1;
        if (abs(dCol)==1 && dRow==1) {
            if (pos->board[dr][dc] != ' ' && isupper(pos->board[dr][dc]))
                return 1;
            if (pos->board[dr][dc]==' ' && is_enpassant_square(pos, dr, dc))
                return 1;
        }
    }
    return 0;
}

int is_valid_move_knight(int sr, int sc, int dr, int dc) {
    int dRow = abs(dr - sr), dCol = abs(dc - sc);
    return ((dRow==2 && dCol==1) || (dRow==1 && dCol==2));
}

int is_valid_move_bishop(FEN *pos, int sr, int sc, int dr, int dc) {
    if (abs(dr - sr) != abs(dc - sc))
        return 0;
    int stepR = (dr - sr) > 0 ? 1 : -1;
    int stepC = (dc - sc) > 0 ? 1 : -1;
    int r = sr + stepR, c = sc + stepC;
    while (r != dr && c != dc) {
        if (pos->board[r][c] != ' ')
            return 0;
        r += stepR;
        c += stepC;
    }
    return 1;
}

int is_valid_move_rook(FEN *pos, int sr, int sc, int dr, int dc) {
    if (sr != dr && sc != dc)
        return 0;
    if (sr == dr) {
        int step = (dc-sc) > 0 ? 1 : -1;
        for (int c = sc + step; c != dc; c += step) {
            if (pos->board[sr][c] != ' ')
                return 0;
        }
    } else {
        int step = (dr-sr) > 0 ? 1 : -1;
        for (int r = sr + step; r != dr; r += step) {
            if (pos->board[r][sc] != ' ')
                return 0;
        }
    }
    return 1;
}

int is_valid_move_queen(FEN *pos, int sr, int sc, int dr, int dc) {
    return is_valid_move_bishop(pos, sr, sc, dr, dc) ||
           is_valid_move_rook(pos, sr, sc, dr, dc);
}

int is_valid_move_king(int sr, int sc, int dr, int dc) {
    return (abs(dr-sr)<=1 && abs(dc-sc)<=1);
}

// General move validation: checks that the destination is not occupied by a friendly piece.
int is_valid_move(FEN *pos, char piece, int sr, int sc, int dr, int dc) {
    char dest = pos->board[dr][dc];
    if (dest != ' ') {
        if (pos->turn=='w' && isupper(dest)) return 0;
        if (pos->turn=='b' && islower(dest)) return 0;
    }
    switch (tolower(piece)) {
        case 'p': return is_valid_move_pawn(pos, sr, sc, dr, dc);
        case 'n': return is_valid_move_knight(sr, sc, dr, dc);
        case 'b': return is_valid_move_bishop(pos, sr, sc, dr, dc);
        case 'r': return is_valid_move_rook(pos, sr, sc, dr, dc);
        case 'q': return is_valid_move_queen(pos, sr, sc, dr, dc);
        case 'k': return is_valid_move_king(sr, sc, dr, dc);
        default: return 0;
    }
}

// ===== APPLY MOVE =====

/*
   This version of apply_move expects moves in algebraic notation and will try to
   perform one of the following:
   
   1. Castling: "O-O" (kingside) or "O-O-O" (queenside).
   2. Pawn moves (including en passant, double moves, and promotions, e.g. "e4", "exd5", "e8Q").
   3. Piece moves (e.g. "Nf3", "Bxc4", etc.).
   
   Note: This implementation uses a board scan to find the moving piece and does not
   handle move disambiguation (e.g. "Nbd2") or check for checks. It also performs only
   basic updates to castling rights.
*/
void apply_move(FEN *position, const char *move) {
    // === CASTLING MOVES ===
    if (strcmp(move, "O-O") == 0 || strcmp(move, "0-0") == 0) {
        // Kingside castling.
        int kingFromRow, kingFromCol, kingToCol, rookFromCol, rookToCol;
        if (position->turn=='w') 
            kingFromRow = 7;
        else
            kingFromRow = 0;
        kingFromCol = 4; kingToCol = 6;
        rookFromCol = 7; rookToCol = 5;
        // Move king.
        position->board[kingFromRow][kingFromCol] = ' ';
        position->board[kingFromRow][kingToCol] = (position->turn=='w') ? 'K' : 'k';
        // Move rook.
        position->board[kingFromRow][rookFromCol] = ' ';
        position->board[kingFromRow][rookToCol] = (position->turn=='w') ? 'R' : 'r';
        // Update castling rights (remove both king and rook rights for that side).
        if (position->turn=='w')
            strcpy(position->castling, strchr(position->castling, 'k') ? "kq" : "-"); // simplified
        else
            strcpy(position->castling, strchr(position->castling, 'K') ? "KQ" : "-");
        // Clear en passant.
        // strcpy(position->enPassant, "-");
    }
    else if (strcmp(move, "O-O-O") == 0 || strcmp(move, "0-0-0") == 0) {
        // Queenside castling.
        int kingFromRow, kingFromCol, kingToCol, rookFromCol, rookToCol;
        if (position->turn=='w') 
            kingFromRow = 7; 
        else if (position->turn=='b')
            kingFromRow = 0;
        kingFromCol = 4; kingToCol = 2;
        rookFromCol = 0; rookToCol = 3;

        position->board[kingFromRow][kingFromCol] = ' ';
        position->board[kingFromRow][kingToCol] = (position->turn=='w') ? 'K' : 'k';
        position->board[kingFromRow][rookFromCol] = ' ';
        position->board[kingFromRow][rookToCol] = (position->turn=='w') ? 'R' : 'r';
        // Update castling rights.
        if (position->turn=='w')
            strcpy(position->castling, strchr(position->castling, 'k') ? "kq" : "-");
        else
            strcpy(position->castling, strchr(position->castling, 'K') ? "KQ" : "-");
        // strcpy(position->enPassant, "-");
    }
    else {
        char piece;       // moving piece letter (capital for white, lowercase for black)
        int destIndex = 0;
        char promotionPiece = '\0';  // if promotion is present
        
        // If the move string starts with one of "KQRBN", assume piece move.
        if (isupper(move[0]) && strchr("KQRBN", move[0]) != NULL) {

            // Checking for elimination/multiple letter move
            piece = (position->turn == 'w') ? move[0] : tolower(move[0]);
            if (strchr(move, 'x')) {
                for (int i = 0; move[i] != '\0'; i++) {
                    if (move[i] == 'x') {
                        destIndex = i+1;
                        break;
                    }
                }
            }
            else if (strlen(move) > 3) {
                if (isalpha(move[1]) && isalpha(move[2])) destIndex = 2;
                else if (isalpha(move[1]) && isdigit(move[2]) && isalpha(move[3])) destIndex = 3;                
            }
            else destIndex = 1;
        } else {
            // Otherwise, assume pawn move.
            piece = (position->turn == 'w') ? 'P' : 'p';
            if (strchr(move, 'x')) {
                for (int i = 0; move[i] != '\0'; i++) {
                    if (move[i] == 'x') {
                        destIndex = i+1;
                        break;
                    }
                }
            }
            // Simple move
            else if (strlen(move) > 2 && !strchr(move, '=')){
                if (isalpha(move[1]) && isalpha(move[2])) destIndex = 2;
                else if (isalpha(move[1]) && isdigit(move[2]) && isalpha(move[3])) destIndex = 3;
            }
            else destIndex = 0;
        }
        
        // Destination square is always given by two characters.
        char file = move[destIndex];
        char rank = move[destIndex+1];
        int toCol = file - 'a';
        int toRow = 8 - (rank - '0');
        
        // If the move string is longer than 2 characters after the piece letter,
        // it might be a promotion (or include an 'x' for capture). For simplicity,
        // we assume that if the last character is an uppercase letter (other than the piece letter)
        // it is a promotion indicator.
        int len = strlen(move);
        if (len > destIndex+2) {
            char lastChar = move[len-1];
            if (strchr("QRBN", lastChar) != NULL) {
                promotionPiece = lastChar;
            }
        }
        
        // Determine which piece to search for.
        char searchPiece = piece;
        
        // Look for a piece on the board that can move to the destination.
        int fromRow = -1, fromCol = -1;
        for (int r = 0; r < BOARD_SIZE; r++) {
            for (int c = 0; c < BOARD_SIZE; c++) {
                if (position->board[r][c] == searchPiece) {
                    if (is_valid_move(position, piece, r, c, toRow, toCol)) {
                        fromRow = r;
                        fromCol = c;
                        goto found;
                    }
                }
            }
        }
found:
        if (fromRow == -1) {
            printf("Invalid move: no %c found that can move to %c%c\n", piece, file, rank);
            exit(EXIT_FAILURE);
        }
        
        // Check if this is an en passant capture.
        int enpassantCapture = 0;
        if (tolower(piece)=='p' && abs(toCol-fromCol)==1 && position->board[toRow][toCol]==' ' &&
            is_enpassant_square(position, toRow, toCol)) {
            enpassantCapture = 1;
        }
        
        // (Optionally) record any captured piece.
        char captured = position->board[toRow][toCol];
        
        // Move the piece.
        position->board[toRow][toCol] = searchPiece;
        position->board[fromRow][fromCol] = ' ';
        
        // Edit bit board
        if (position->turn == 'w') {
            SET_BIT(whitePieces, (7 - toRow) * 8 + toCol);
            CLEAR_BIT(blackPieces, (7 - toRow) * 8 + toCol);
            CLEAR_BIT(whitePieces, (7 - fromRow) * 8 + fromCol);
        }
        else {
            SET_BIT(blackPieces, (7 - toRow) * 8 + toCol);
            CLEAR_BIT(whitePieces, (7 - toRow) * 8 + toCol);
            CLEAR_BIT(blackPieces, (7 - fromRow) * 8 + fromCol);         
        }
        pieces = blackPieces | whitePieces;
        // For en passant capture, remove the pawn that is captured.
        if (enpassantCapture) {
            int capRow = (position->turn=='w') ? toRow+1 : toRow-1;
            captured = position->board[capRow][toCol];
            position->board[capRow][toCol] = ' ';
        }
        
        // Handle pawn promotion.
        if (tolower(searchPiece)=='p') {
            if ((position->turn=='w' && toRow==0) ||
                (position->turn=='b' && toRow==7)) {
                // If no promotion piece was given, default to queen.
                if (promotionPiece == '\0')
                    promotionPiece = 'Q';
                // Make sure promotion piece has correct case.
                promotionPiece = (position->turn=='w') ? promotionPiece : tolower(promotionPiece);
                position->board[toRow][toCol] = promotionPiece;
            }
        }
        
        // Update the half–move clock.
        if (tolower(searchPiece)=='p' || captured != ' ')
            position->halfMoveClock = 0;
        else
            position->halfMoveClock++;
        
        // Handle en passant target: only set if pawn moved two squares forward.
        if (tolower(searchPiece)=='p' && abs(toRow - fromRow)==2) {
            if (toCol > 0) {
                if ((searchPiece == 'p' && position->board[toRow][toCol-1] == 'P') || (searchPiece == 'P' && position->board[toRow][toCol-1] == 'p')) {
                    int epRow = (toRow + fromRow)/2;
                    char epSquare[3];
                    coord_to_square(epRow, toCol, epSquare);
                    strcpy(position->enPassant, epSquare);
                    goto jump;
                }
                else strcpy(position->enPassant, "-");
            }
            if (toCol < BOARD_SIZE-1) {
                if ((searchPiece == 'p' && position->board[toRow][toCol+1] == 'P') || (searchPiece == 'P' && position->board[toRow][toCol+1] == 'p')) {
                    int epRow = (toRow + fromRow)/2;
                    char epSquare[3];
                    coord_to_square(epRow, toCol, epSquare);
                    strcpy(position->enPassant, epSquare);
                }
                else strcpy(position->enPassant, "-");                
            }
            
        } else {
            strcpy(position->enPassant, "-");
        }
jump:        
        // (Simplified update to castling rights: if a king moves, remove castling rights.
        if (tolower(searchPiece)=='k') {
            char newCastling[5] = "";
            if (position->turn == 'w') {
                // Remove white castling rights ('K' and 'Q'), keep black rights.
                for (unsigned long int i = 0; i < strlen(position->castling); i++) {
                    if (position->castling[i] == 'k' || position->castling[i] == 'q') {
                        strncat(newCastling, &position->castling[i], 1);
                    }
                }
            } else {  // Black's turn.
                // Remove black castling rights ('k' and 'q'), keep white rights.
                for (unsigned long int i = 0; i < strlen(position->castling); i++) {
                    if (position->castling[i] == 'K' || position->castling[i] == 'Q') {
                        strncat(newCastling, &position->castling[i], 1);
                    }
                }
            }
            strcpy(position->castling, newCastling);
        }

        // If a rook moves from its original square, remove its castling right.
        if (tolower(searchPiece)=='r') {
            if ((position->turn=='w' && fromRow==7 && fromCol==0) ||
                (position->turn=='b' && fromRow==0 && fromCol==0)) {
                // remove queenside right.
                char *p = strchr(position->castling, (position->turn=='w' ? 'Q' : 'q'));
                if (p) *p = ' ';
            }
            if ((position->turn=='w' && fromRow==7 && fromCol==7) ||
                (position->turn=='b' && fromRow==0 && fromCol==7)) {
                // remove kingside right.
                char *p = strchr(position->castling, (position->turn=='w' ? 'K' : 'k'));
                if (p) *p = ' ';
            }
        }
    }
    
    // Switch turn.
    position->turn = (position->turn=='w') ? 'b' : 'w';
    
    // Increment full move number after Black’s move.
    if (position->turn=='w')
        position->fullMoveNum++;
}

// ===== MAIN FUNCTION FOR TESTING =====

FEN new_fen(FEN initialFEN, const char *move) {
    apply_move(&initialFEN, move);
    return initialFEN;
}

