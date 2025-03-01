#ifndef find_moves_h
#define find_moves_h
#include <stdint.h>
typedef uint64_t U64;
void find_moves(const FEN fen, char *** moves, int * move_count);
void initNonSlidingMoves();
void printBitboard(char piece, int square, U64 bb, U64 opps, char *** moves, int * move_count, int enpassant);
#endif