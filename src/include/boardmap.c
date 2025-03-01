#include <stdio.h>
#include "Headers/boardmap.h"
#include <stdint.h>

U64 whitePieces = 0x000000000000FFFFULL;
U64 blackPieces   = 0xFFFF000000000000ULL;
U64 pieces;
void initialize_board() {
    pieces = whitePieces | blackPieces;
}
U64 getGlobalVar(char color) {
    if (color == 'w'){
        return whitePieces;
    }else if (color == 'b'){
        return blackPieces;
    }else{
        return pieces;
    }
}
// edit_board(const int start, const int target) {

// }