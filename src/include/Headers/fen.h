#ifndef fen_h
#define fen_h

typedef struct {
    char board[8][8];
    char turn;
    char castling[5];
    char enPassant[5];
    int halfMoveClock;
    int fullMoveNum;
} FEN;



void parse_fen(const char *fen, FEN *position);
FEN new_fen(FEN initialFEN, const char *move);
void coord_to_square(int row, int col, char *square);
#endif