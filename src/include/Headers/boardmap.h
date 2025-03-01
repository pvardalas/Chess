#ifndef boardmap_h
#define boardmap_h
#include <stdint.h>

/* Helper macros */
#define SET_BIT(bb, sq) ((bb) |= (1ULL << (sq)))
#define GET_BIT(bb, sq) ((bb) & (1ULL << (sq)))
#define CLEAR_BIT(bb, sq) ((bb) &= ~(1ULL << (sq)))

/*
Code Snippet for BitBoard
----------------------------------------------------
*/
// #define U64 uint64_t
typedef uint64_t U64;
extern U64 whitePieces; // Map for white pieces
extern U64 blackPieces; // Map for black pieces
extern U64 pieces;      // Map for all pieces
/*
----------------------------------------------------
*/

void initialize_board();
// void edit_board(const int start, const int target);
U64 getGlobalVar(char color);
#endif