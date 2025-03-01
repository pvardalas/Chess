#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "Headers/fen.h"
#include "Headers/find_moves.h"
#include "Headers/boardmap.h"
#include <ctype.h>
#define U64 uint64_t

int get_en_passant_square(const char *enPassant) {
    if (enPassant[0] == '-') return -1;  // No en passant
    int file = enPassant[0] - 'a';  // Convert 'a'-'h' to 0-7
    int rank = enPassant[1] - '1';  // Convert '1'-'8' to 0-7
    return (rank * 8) + file;
}
void printBitboard(char piece, int from_square, U64 bb, U64 opps, char *** moves, int * move_count, int enpassant) {
    for (int rank = 7; rank >= 0; rank--) {
        for (int file = 0; file < 8; file++) {
            int sq = rank * 8 + file;
            if (GET_BIT(bb, sq)) {
                if (piece == 'P' && ((abs(sq - from_square) == 9 || abs(sq - from_square) == 7) && (!GET_BIT(opps, sq) && enpassant != sq))) continue;
                (*move_count)++;
                char **temp = realloc(*moves, (*move_count) * sizeof(char *));
                if (!temp) {
                    perror("Realloc failed");
                    exit(1); // Stop execution if realloc fails
                }
                *moves = temp;
                (*moves)[(*move_count)-1] = malloc(6 * sizeof(char));
                if (piece == 'P') {  // Pawn-specific printing
                    // if ((abs(sq - from_square) == 9 || abs(sq - from_square) == 7) && sq != enpassant) continue;
                    // For pawn moves, use the pawn's originating file letter.
                    char originFile = 'a' + (from_square % 8);
                    if (GET_BIT(opps, sq) || sq == enpassant)
                        // Capture: e.g., "exd4"
                        sprintf((*moves)[(*move_count)-1], "%cx%c%c", originFile, 'a' + file, '1' + rank);
                    else
                        // Quiet move: e.g., "ef4" (your custom notation that includes the origin file)
                        sprintf((*moves)[(*move_count)-1], "%c%c", 'a' + file, '1' + rank);
                }
                else {
                    if (GET_BIT(opps, sq))
                        sprintf((*moves)[(*move_count)-1], "%cx%c%c", piece, 'a'+file, '1'+rank);
                    else
                        sprintf((*moves)[(*move_count)-1], "%c%c%c", piece, 'a'+file, '1'+rank);                    
                }
            }
        }
    }
}
/* Precomputed knight attack masks */
U64 knightAttacks[64] = {0};

/* Precomputed king attack masks */
U64 kingAttacks[64] = {0};

U64 filterKingMoves(int square, U64 allyPieces) {
    U64 validMoves = kingAttacks[square] & ~allyPieces;
    return validMoves;
}
U64 filterKnightMoves(int square, U64 allyPieces) {
    U64 validMoves = knightAttacks[square] & ~allyPieces;
    return validMoves;
}
/* Compute knight moves for a given square.
   The knight can move in 8 L-shaped directions. */
U64 computeKnightAttacks(int square) {
    int rank = square / 8;
    int file = square % 8;
    U64 attacks = 0ULL;
    int offsets[8][2] = {
        { 2,  1}, { 1,  2}, {-1,  2}, {-2,  1},
        {-2, -1}, {-1, -2}, { 1, -2}, { 2, -1}
    };
    for (int i = 0; i < 8; i++) {
        int r = rank + offsets[i][0];
        int f = file + offsets[i][1];
        if (r >= 0 && r < 8 && f >= 0 && f < 8) {
            int targetSquare = r * 8 + f;
            SET_BIT(attacks, targetSquare);
        }
    }
    return attacks;
}

/* Compute king moves for a given square.
   The king moves one square in any direction. */
U64 computeKingAttacks(int square) {
    // int rank = square / 8;
    // int file = square % 8;
    // U64 attacks = 0ULL;
    // int offsets[8][2] = {
    //     { 1,  0}, { 1,  1}, { 0,  1}, {-1,  1},
    //     {-1,  0}, {-1, -1}, { 0, -1}, { 1, -1}
    // };
    // for (int i = 0; i < 8; i++) {
    //     int r = rank + offsets[i][0];
    //     int f = file + offsets[i][1];
    //     if (r >= 0 && r < 8 && f >= 0 && f < 8) {
    //         int targetSquare = r * 8 + f;
    //         SET_BIT(attacks, targetSquare);
    //     }
    // }
    // return attacks;
    U64 king = 1ULL << square;  // Position of the king
    U64 attacks = 0ULL;

    // Masks to prevent wrapping from file A/H
    U64 notAFile = 0xfefefefefefefefeULL;  // clear bits on file A
    U64 notHFile = 0x7f7f7f7f7f7f7f7fULL;  // clear bits on file H

    // Cardinal directions (up and down)
    attacks |= king << 8;                // north (e.g., move to e2 from e1)
    attacks |= king >> 8;                // south (e.g., move to e1 from e2)

    // Horizontal moves (left and right)
    attacks |= (king & notHFile) << 1;   // east (e.g., move to f1 from e1)
    attacks |= (king & notAFile) >> 1;   // west (e.g., move to d1 from e1)

    // Diagonal moves
    attacks |= (king & notHFile) << 9;   // north-east (e.g., move to f2 from e1)
    attacks |= (king & notAFile) << 7;   // north-west (e.g., move to d2 from e1)
    attacks |= (king & notHFile) >> 7;   // south-east (e.g., move to f0 from e1)
    attacks |= (king & notAFile) >> 9;   // south-west (e.g., move to d0 from e1)

    return attacks;
}

/* Initialize the knight and king move tables. */
void initNonSlidingMoves() {
    for (int sq = 0; sq < 64; sq++) {
        knightAttacks[sq] = computeKnightAttacks(sq);
        kingAttacks[sq] = computeKingAttacks(sq);
    }
}

/* Bishop moves: slide diagonally until blocked.
   Includes the enemy square if encountered, then stops. */
U64 computeBishopAttacks(int square, U64 occupancy, U64 allies) {
    U64 moves = 0ULL;
    int rank = square / 8;
    int file = square % 8;
    int directions[4][2] = { { 1,  1}, { 1, -1}, { -1,  1}, { -1, -1} };

    for (int d = 0; d < 4; d++) {
        int dr = directions[d][0];
        int df = directions[d][1];
        int r = rank, f = file;
        while (1) {
            r += dr;
            f += df;
            if (r < 0 || r >= 8 || f < 0 || f >= 8)
                break;
            int targetSquare = r * 8 + f;
            moves |= (1ULL << targetSquare);
            if (occupancy & (1ULL << targetSquare))
                break;
        }
    }
    moves &= ~allies;  // Remove moves landing on ally pieces
    return moves;
}

/* Rook moves: slide vertically and horizontally until blocked */
U64 computeRookAttacks(int square, U64 occupancy, U64 allies) {
    U64 moves = 0ULL;
    int rank = square / 8;
    int file = square % 8;
    int directions[4][2] = { { 1, 0}, { -1, 0}, { 0, 1}, { 0, -1} };

    for (int d = 0; d < 4; d++) {
        int dr = directions[d][0];
        int df = directions[d][1];
        int r = rank, f = file;
        while (1) {
            r += dr;
            f += df;
            if (r < 0 || r >= 8 || f < 0 || f >= 8)
                break;
            int targetSquare = r * 8 + f;
            moves |= (1ULL << targetSquare);
            if (occupancy & (1ULL << targetSquare))
                break;
        }
    }
    moves &= ~allies;
    return moves;
}

/* Queen moves: union of bishop and rook moves */
U64 computeQueenAttacks(int square, U64 occupancy, U64 allies) {
    return computeBishopAttacks(square, occupancy, allies) | computeRookAttacks(square, occupancy, allies);
}

/* White pawn moves (bitboard representation).
   - Advance one square if empty.
   - Advance two squares from starting rank (rank 2: bits 8-15) if both squares are empty.
   - Diagonal captures (the printBitboard function will add an 'x' if a target square is occupied by an opponent).
*/
U64 computeWhitePawnMoves(int square, U64 occupancy) {
    U64 moves = 0ULL;
    int oneStep = square + 8;
    if (oneStep < 64 && !(occupancy & (1ULL << oneStep))) {
        moves |= (1ULL << oneStep);
        if (square >= 8 && square < 16) {  // starting rank for white pawns
            int twoStep = square + 16;
            if (!(occupancy & (1ULL << twoStep)))
                moves |= (1ULL << twoStep);
        }
    }
    // Diagonal captures (note: these moves are added even if the square is empty;
    // printBitboard uses the opps bitboard to decide whether to print 'x')
    if ((square % 8) != 7) {  // not on file H
        int captureRight = square + 9;
        if (captureRight < 64)
            moves |= (1ULL << captureRight);
    }
    if ((square % 8) != 0) {  // not on file A
        int captureLeft = square + 7;
        if (captureLeft < 64)
            moves |= (1ULL << captureLeft);
    }
    return moves;
}

/* Black pawn moves */
U64 computeBlackPawnMoves(int square, U64 occupancy) {
    U64 moves = 0ULL;
    int oneStep = square - 8;
    if (oneStep >= 0 && !(occupancy & (1ULL << oneStep))) {
        moves |= (1ULL << oneStep);
        if (square >= 48 && square < 56) {  // starting rank for black pawns (rank 7)
            int twoStep = square - 16;
            if (!(occupancy & (1ULL << twoStep)))
                moves |= (1ULL << twoStep);
        }
    }
    if ((square % 8) != 7) {
        int captureRight = square - 7;
        if (captureRight >= 0)
            moves |= (1ULL << captureRight);
    }
    if ((square % 8) != 0) {
        int captureLeft = square - 9;
        if (captureLeft >= 0)
            moves |= (1ULL << captureLeft);
    }
    return moves;
}
int get_castling_bitmask(const char *castling_str) {
    int mask = 0;
    if (strchr(castling_str, 'K')) mask |= 1;
    if (strchr(castling_str, 'Q')) mask |= 2;
    if (strchr(castling_str, 'k')) mask |= 4;
    if (strchr(castling_str, 'q')) mask |= 8;
    return mask;
}
void add_castling_moves(const FEN *board_fen, char ***moves, int *move_count) {
    int castlingRights = get_castling_bitmask(board_fen->castling);
    if (board_fen->turn == 'w') {
        // int kingSquare = 4;  // White king on e1 (index 4)
        // Kingside: squares f1 (index 5) and g1 (index 6)
        if (castlingRights & 1) {
            if (board_fen->board[7][5] == ' ' && board_fen->board[7][6] == ' ') {
                (*move_count)++;
                char **temp = realloc(*moves, (*move_count) * sizeof(char *));
                if (!temp) { perror("Realloc failed"); exit(1); }
                *moves = temp;
                (*moves)[(*move_count)-1] = strdup("O-O");
            }
        }
        // Queenside: squares d1 (index 3) and c1 (index 2) must be clear.
        if (castlingRights & 2) {
            if (board_fen->board[7][1] == ' ' && board_fen->board[7][2] == ' ' &&
                board_fen->board[7][3] == ' ') {
                (*move_count)++;
                char **temp = realloc(*moves, (*move_count) * sizeof(char *));
                if (!temp) { perror("Realloc failed"); exit(1); }
                *moves = temp;
                (*moves)[(*move_count)-1] = strdup("O-O-O");
            }
        }
    } else {
        // Black castling: king on e8 (index 60)
        // int kingSquare = 60;
        if (castlingRights & 4) {
            // Kingside: squares f8 (61) and g8 (62)
            if (board_fen->board[0][5] == ' ' && board_fen->board[0][6] == ' ') {
                (*move_count)++;
                char **temp = realloc(*moves, (*move_count) * sizeof(char *));
                if (!temp) { perror("Realloc failed"); exit(1); }
                *moves = temp;
                (*moves)[(*move_count)-1] = strdup("O-O");
            }
        }
        if (castlingRights & 8) {
            // Queenside: squares d8 (59) and c8 (58)
            if (board_fen->board[0][1] == ' ' && board_fen->board[0][2] == ' ' &&
                board_fen->board[0][3] == ' ') {
                (*move_count)++;
                char **temp = realloc(*moves, (*move_count) * sizeof(char *));
                if (!temp) { perror("Realloc failed"); exit(1); }
                *moves = temp;
                (*moves)[(*move_count)-1] = strdup("O-O-O");
            }
        }
    }
}
void find_moves(const FEN board_fen, char *** moves, int * move_count) {
    (*move_count) = 0;
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            if (board_fen.board[i][j] == ' ') continue;
            if ((board_fen.turn == 'w' && board_fen.board[i][j] == tolower(board_fen.board[i][j])) || (board_fen.turn == 'b' && board_fen.board[i][j] == toupper(board_fen.board[i][j]))) continue;
            int square = (7-i)*8 + j;
            U64 allies = (board_fen.turn == 'w') ? whitePieces : blackPieces;
            U64 opps = (board_fen.turn == 'w') ? blackPieces : whitePieces;
            U64 occupancy = whitePieces | blackPieces;
            int enpassant = get_en_passant_square(board_fen.enPassant);
            if ((board_fen.turn == 'w' && board_fen.board[i][j] == 'K') || (board_fen.turn == 'b' && board_fen.board[i][j] == 'k')) {
                printBitboard(toupper(board_fen.board[i][j]), square, filterKingMoves(square, allies), opps, moves, move_count, enpassant);
                add_castling_moves(&board_fen, moves, move_count);
            }
            else if ((board_fen.turn == 'w' && board_fen.board[i][j] == 'N') || (board_fen.turn == 'b' && board_fen.board[i][j] == 'n'))
                printBitboard(toupper(board_fen.board[i][j]), square, filterKnightMoves(square, allies), opps, moves, move_count, enpassant);                 
            else if (toupper(board_fen.board[i][j]) == 'B') {
                U64 moves_bb = computeBishopAttacks(square, occupancy, allies);
                printBitboard(toupper(board_fen.board[i][j]), square, moves_bb, opps, moves, move_count, enpassant);
            }
            else if (toupper(board_fen.board[i][j]) == 'R') {
                U64 moves_bb = computeRookAttacks(square, occupancy, allies);
                printBitboard(toupper(board_fen.board[i][j]), square, moves_bb, opps, moves, move_count, enpassant);
            }
            else if (toupper(board_fen.board[i][j]) == 'Q') {
                U64 moves_bb = computeQueenAttacks(square, occupancy, allies);
                printBitboard(toupper(board_fen.board[i][j]), square, moves_bb, opps, moves, move_count, enpassant);
            }
            else if (toupper(board_fen.board[i][j]) == 'P') {
                U64 moves_bb;
                if (board_fen.turn == 'w')
                    moves_bb = computeWhitePawnMoves(square, occupancy);
                else
                    moves_bb = computeBlackPawnMoves(square, occupancy);
                printBitboard(toupper(board_fen.board[i][j]), square, moves_bb, opps, moves, move_count, enpassant);
            }
        }
    }
}