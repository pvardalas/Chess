#include "Headers/move.h"
#include <stdio.h>
#include <stdlib.h>

int moves_count(char * moves) {
    int count = 0;
    char * ptr = moves;
    while (*ptr) {
        if (*ptr == ' ') count++;
        ptr++;
    }
    return count+1;
}

int move(char * fen, char * moves) {
    if (!fen || !moves) exit(EXIT_FAILURE);
    return moves_count(moves);
}