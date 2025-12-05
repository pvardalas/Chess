#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>
#include <ctype.h>
#include <time.h>
#include "Headers/fen.h"

#define PAWN_VALUE 1
#define KNIGHT_VALUE 3
#define BISHOP_VALUE 3
#define ROOK_VALUE 5
#define QUEEN_VALUE 9
#define KING_VALUE 100

typedef struct
{
    int fromX, fromY, toX, toY;
    char piece ;         // 'P', 'N', 'B', 'R', 'Q', 'K'
    bool isCapture;     // true if a capture
    char promotion;     // promotion piece if any (e.g. 'Q'), or '\0'
} Move;

typedef struct node
{
    char board[8][8];
    Move move;
    long long int moveScore;
    struct node **children;
    int childcount;
} Node;
// Global deadline variable (set in iterativeDeepening)
static struct timespec search_deadline;

// Helper: returns true if the current time is past the deadline.
bool time_is_up() {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    if (now.tv_sec > search_deadline.tv_sec ||
        (now.tv_sec == search_deadline.tv_sec && now.tv_nsec >= search_deadline.tv_nsec)) {
        return true;
    }
    return false;
}
int getPieceValue(char piece)
{
    switch (toupper(piece))
    {
    case 'P':
        return PAWN_VALUE;
    case 'N':
        return KNIGHT_VALUE;
    case 'B':
        return BISHOP_VALUE;
    case 'R':
        return ROOK_VALUE;
    case 'Q':
        return QUEEN_VALUE;
    case 'K':
        return KING_VALUE;
    default:
        return 0;
    }
}

int compareMoves(const void *a, const void *b)
{
    Node *nodeA = *(Node **)a;
    Node *nodeB = *(Node **)b;
    return (nodeB->moveScore - nodeA->moveScore);
}

// ------------------------- FEN Parsing ------------------------- //
void parseFEN()
{
}

// ------------------------- Move Conversion ------------------------- //







// decodeMove takes the board, a move in SAN, a pointer to a Move structure,
// and the current player ('w' or 'b'). It returns true if the move is decoded successfully.
bool decodeMove(char board[8][8], const char *moveStr, Move *move, char currentPlayer) {
    // Initialize capture flag and promotion.
    move->isCapture = false;
    move->promotion = '\0';

    int len = strlen(moveStr);
    if (len < 2)
        return false;

    // First, check for castling.
    if (strcmp(moveStr, "O-O") == 0 || strcmp(moveStr, "0-0") == 0 ||
        strcmp(moveStr, "O-O-O") == 0 || strcmp(moveStr, "0-0-0") == 0) {
        // For castling, we assume the moving piece is the king.
        move->piece = (currentPlayer == 'w') ? 'K' : 'k';
        if (currentPlayer == 'w') {
            move->fromX = 7; move->fromY = 4;
            if (moveStr[1] == '-') {  // kingside castling
                move->toX = 7; move->toY = 6;
            } else { // queenside castling ("O-O-O")
                move->toX = 7; move->toY = 2;
            }
        } else {
            move->fromX = 0; move->fromY = 4;
            if (moveStr[1] == '-') {
                move->toX = 0; move->toY = 6;
            } else {
                move->toX = 0; move->toY = 2;
            }
        }
        return true;
    }

    // Look for a promotion indicator. If found, record the promotion piece
    // and adjust the effective length of the move string (excluding the promotion part).
    int effectiveLen = len;
    const char *promoPtr = strchr(moveStr, '=');
    if (promoPtr != NULL) {
        move->promotion = *(promoPtr + 1);
        effectiveLen = promoPtr - moveStr;  // characters before '=' are part of the move
    }

    // The destination square is always the last two characters of the effective move string.
    if (effectiveLen < 2)
        return false;
    char file = moveStr[effectiveLen - 2];
    char rank = moveStr[effectiveLen - 1];
    if (file < 'a' || file > 'h' || rank < '1' || rank > '8')
        return false;
    int destY = file - 'a';
    int destX = '8' - rank;

    // Determine the prefix part of the move (everything except the destination square).
    int prefixLen = effectiveLen - 2;

    // Decide whether it’s a piece move or a pawn move.
    // In SAN, if the first character is one of "KQRBN", it’s a piece move.
    if (prefixLen > 0 && strchr("KQRBN", moveStr[0]) != NULL) {
        // Piece move.
        move->piece = (currentPlayer == 'w') ? moveStr[0] : tolower(moveStr[0]);
        // If there is an 'x' anywhere in the move string, mark it as a capture.
        if (strchr(moveStr, 'x') != NULL)
            move->isCapture = true;
        // In a full implementation you would handle any disambiguation characters here.
        // Now search the board for a matching piece that can reach the destination.
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
                if (board[i][j] == move->piece) {
                    // (Here you would normally check if the piece can move to destX,destY.)
                    move->fromX = i;
                    move->fromY = j;
                    move->toX = destX;
                    move->toY = destY;
                    return true;
                }
            }
        }
        return false;  // No matching piece found.
    } else {
        // Pawn move.
        move->piece = (currentPlayer == 'w') ? 'P' : 'p';
        // Check if this is a pawn capture move.
        // For pawn captures, the SAN is of the form "exd5" (prefixLen would be 2 or 3).
        if (prefixLen >= 2 && moveStr[1] == 'x') {
            move->isCapture = true;
            // The first character gives the starting file.
            int fromFile = moveStr[0] - 'a';
            // Determine fromX based on pawn movement.
            int candidateFromX1 = (currentPlayer == 'w') ? destX + 1 : destX - 1;
            int candidateFromX2 = (currentPlayer == 'w') ? 6 : 1;  // pawn's starting row
            if (candidateFromX1 >= 0 && candidateFromX1 < 8 && board[candidateFromX1][fromFile] == move->piece) {
                move->fromX = candidateFromX1;
                move->fromY = fromFile;
            } else if (candidateFromX2 >= 0 && candidateFromX2 < 8 && board[candidateFromX2][fromFile] == move->piece) {
                move->fromX = candidateFromX2;
                move->fromY = fromFile;
            } else {
                return false;
            }
        } else {
            // Normal pawn move (non-capture). In SAN, these moves have only the destination square.
            int startRow = (currentPlayer == 'w') ? destX + 1 : destX - 1;
            if (startRow >= 0 && startRow < 8 && board[startRow][destY] == move->piece) {
                move->fromX = startRow;
                move->fromY = destY;
            } else {
                // Check for the possibility of a double move.
                if (currentPlayer == 'w' && destX == 3 && board[6][destY] == move->piece) {
                    move->fromX = 6;
                    move->fromY = destY;
                } else if (currentPlayer == 'b' && destX == 4 && board[1][destY] == move->piece) {
                    move->fromX = 1;
                    move->fromY = destY;
                } else {
                    return false;
                }
            }
        }
        move->toX = destX;
        move->toY = destY;
        return true;
    }
}


void moveToAlgebraicString(Move move, char *outStr)
{
    int pos = 0;

    // Handle castling (assuming king move with a two-file horizontal move)
    if (move.piece == 'K' && abs(move.fromY - move.toY) == 2)
    {
        if (move.toY > move.fromY)
            strcpy(outStr, "O-O");    // kingside castle
        else
            strcpy(outStr, "O-O-O");  // queenside castle
        return;
    }

    // For non-pawn moves, add the piece letter.
    // Note: In SAN, pawn moves normally do not include a letter.
    if (move.piece != 'P')
    {
        outStr[pos++] = move.piece;
    }
    else
    {
        // For pawn captures, include originating file.
        if (move.isCapture)
            outStr[pos++] = 'a' + move.fromY;
    }

    // Add capture indicator if needed.
    if (move.isCapture)
    {
        outStr[pos++] = 'x';
    }

    // Append destination square (file then rank)
    outStr[pos++] = 'a' + move.toY;
    outStr[pos++] = '8' - move.toX;

    // Handle promotions.
    if (move.promotion != '\0')
    {
        outStr[pos++] = '=';
        outStr[pos++] = move.promotion;
    }

    // Terminate string.
    outStr[pos] = '\0';
}

long long int evaluateBoard(char board[8][8]);
// ------------------------- Tree Functions ------------------------- //
Node *createNode(char board[8][8], Move move)
{
    Node *newNode = malloc(sizeof(Node));
    memcpy(newNode->board, board, 64);
    newNode->move = move;
    newNode->children = NULL;
    newNode->childcount = 0;
    newNode->moveScore = evaluateBoard(newNode->board);
    return newNode;
}

void addChild(Node *parent, Node *child)
{
    parent->children = realloc(parent->children, (parent->childcount + 1) * sizeof(Node *));
    parent->children[parent->childcount++] = child;
}

void freeTree(Node *root)
{
    if (!root)
        return;
    for (int i = 0; i < root->childcount; i++)
        freeTree(root->children[i]);
    free(root->children);
    free(root);
}

// ------------------------- Evaluation ------------------------- //
long long int evaluateBoard(char board[8][8])
{
    long long score = 0;
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            switch (board[i][j])
            {
            case 'P':
                score += PAWN_VALUE;
                break;
            case 'N':
                score += KNIGHT_VALUE;
                break;
            case 'B':
                score += BISHOP_VALUE;
                break;
            case 'R':
                score += ROOK_VALUE;
                break;
            case 'Q':
                score += QUEEN_VALUE;
                break;
            case 'K':
                score += KING_VALUE;
                break;
            case 'p':
                score -= PAWN_VALUE;
                break;
            case 'n':
                score -= KNIGHT_VALUE;
                break;
            case 'b':
                score -= BISHOP_VALUE;
                break;
            case 'r':
                score -= ROOK_VALUE;
                break;
            case 'q':
                score -= QUEEN_VALUE;
                break;
            case 'k':
                score -= KING_VALUE;
                break;
            }
        }
    }
    return score;
}

void applyMove(char src[8][8], char dest[8][8], Move move)
{
    memcpy(dest, src, 64);
    dest[move.toX][move.toY] = dest[move.fromX][move.fromY];
    dest[move.fromX][move.fromY] = ' ';
}

// ------------------------- Transposition Table ------------------------- //
typedef struct
{
    unsigned long long key;
    long long evaluation;
    int depth;
} TTEntry;

#define TT_SIZE 10007
TTEntry ttTable[TT_SIZE];

void initTT() { memset(ttTable, 0, sizeof(ttTable)); }

unsigned long long computeHash(char board[8][8], char currentPlayer)
{
    unsigned long long hash = 1469598103934665603ULL;
    hash ^= (unsigned long long)currentPlayer;
    hash *= 1099511628211ULL;
    for (int i = 0; i < 8; i++)
        for (int j = 0; j < 8; j++)
            hash = (hash ^ board[i][j]) * 1099511628211ULL;
    return hash;
}

bool ttLookup(unsigned long long key, int depth, long long *eval)
{
    TTEntry *entry = &ttTable[key % TT_SIZE];
    if (entry->key == key && entry->depth >= depth)
    {
        *eval = entry->evaluation;
        return true;
    }
    return false;
}

void ttStore(unsigned long long key, int depth, long long eval)
{
    TTEntry *entry = &ttTable[key % TT_SIZE];
    entry->key = key;
    entry->evaluation = eval;
    entry->depth = depth;
}

// ------------------------- QUIESCENCE SEARCH ------------------------- //

// struct to save capture moves and new board states
typedef struct
{
    Move move;
    char newBoard[8][8];
} CaptureNode;

// function to check if a piece is of the enemy
bool isOpponentPiece(char piece, char currentPlayer)
{
    if (piece == ' ')
        return false;
    if (currentPlayer == 'w')
        return islower(piece);
    else
        return isupper(piece);
}

void addCaptureMove(CaptureNode **moves, int *count, char board[8][8], int fromX, int fromY, int toX, int toY)
{
    CaptureNode temp;

    temp.move.fromX = fromX;
    temp.move.fromY = fromY;
    temp.move.toX = toX;
    temp.move.toY = toY;

    // copying current board and playing the move
    memcpy(temp.newBoard, board, 64 * sizeof(char));

    temp.newBoard[toX][toY] = board[fromX][fromY];

    temp.newBoard[fromX][fromY] = ' ';

    *moves = realloc(*moves, ((*count) + 1) * sizeof(CaptureNode));

    (*moves)[*count] = temp;

    (*count)++;
}

CaptureNode *generateCaptureMoves(char board[8][8], char currentPlayer, int *count)
{
    CaptureNode *moves = NULL;
    *count = 0;

    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            char piece = board[i][j];
            if (piece == ' ')
                continue;
            if (currentPlayer == 'w' && !isupper(piece))
                continue;
            if (currentPlayer == 'b' && !islower(piece))
                continue;
            switch (piece)
            {

                // ------------------------ Pawn moves  ------------------------

            case 'P': // white Pawn
                if (i - 1 >= 0)
                {
                    if (j - 1 >= 0 && isOpponentPiece(board[i - 1][j - 1], currentPlayer))
                    {
                        addCaptureMove(&moves, count, board, i, j, i - 1, j - 1);
                    }
                    if (j + 1 < 8 && isOpponentPiece(board[i - 1][j + 1], currentPlayer))
                    {
                        addCaptureMove(&moves, count, board, i, j, i - 1, j + 1);
                    }
                }
                break;
            case 'p': // black Pawn
                if (i + 1 < 8)
                {
                    if (j - 1 >= 0 && isOpponentPiece(board[i + 1][j - 1], currentPlayer))
                    {
                        addCaptureMove(&moves, count, board, i, j, i + 1, j - 1);
                    }
                    if (j + 1 < 8 && isOpponentPiece(board[i + 1][j + 1], currentPlayer))
                    {
                        addCaptureMove(&moves, count, board, i, j, i + 1, j + 1);
                    }
                }
                break;

                // ------------------------ Knight moves ------------------------

            case 'N': // white Knight
            case 'n': // black Knight
            {
                int knightMoves[8][2] = {
                    {-2, -1}, {-2, 1}, {-1, -2}, {-1, 2}, {1, -2}, {1, 2}, {2, -1}, {2, 1}};
                for (int k = 0; k < 8; k++)
                {
                    int newX = i + knightMoves[k][0];
                    int newY = j + knightMoves[k][1];
                    if (newX >= 0 && newX < 8 && newY >= 0 && newY < 8)
                    {
                        if (isOpponentPiece(board[newX][newY], currentPlayer))
                        {
                            addCaptureMove(&moves, count, board, i, j, newX, newY);
                        }
                    }
                }
            }
            break;

                // ------------------------ Bishop moves ------------------------

            case 'B': // white Bishop
            case 'b': // black Bishop
            {
                int directions[4][2] = {
                    {-1, -1}, {-1, 1}, {1, -1}, {1, 1}};
                for (int d = 0; d < 4; d++)
                {
                    int step = 1;
                    while (1)
                    {
                        int newX = i + step * directions[d][0];
                        int newY = j + step * directions[d][1];
                        if (newX < 0 || newX >= 8 || newY < 0 || newY >= 8)
                        {
                            break;
                        }
                        if (board[newX][newY] == ' ')
                        {
                            step++;
                            continue;
                        }
                        if (isOpponentPiece(board[newX][newY], currentPlayer))
                        {
                            addCaptureMove(&moves, count, board, i, j, newX, newY);
                        }
                        break;
                    }
                }
            }
            break;

                // ------------------------ Rook moves ------------------------

            case 'R': // white Rook
            case 'r': // black Rook
            {
                int directions[4][2] = {
                    {-1, 0}, {1, 0}, {0, -1}, {0, 1}};
                for (int d = 0; d < 4; d++)
                {
                    int step = 1;
                    while (1)
                    {
                        int newX = i + step * directions[d][0];
                        int newY = j + step * directions[d][1];
                        if (newX < 0 || newX >= 8 || newY < 0 || newY >= 8)
                        {
                            break;
                        }
                        if (board[newX][newY] == ' ')
                        {
                            step++;
                            continue;
                        }
                        if (isOpponentPiece(board[newX][newY], currentPlayer))
                        {
                            addCaptureMove(&moves, count, board, i, j, newX, newY);
                        }
                        break;
                    }
                }
            }
            break;

                // ------------------------ Queen moves ------------------------

            case 'Q': // white Queen
            case 'q': // black Queen
            {
                int directions[8][2] = {
                    {-1, 0}, {1, 0}, {0, -1}, {0, 1}, {-1, -1}, {-1, 1}, {1, -1}, {1, 1}};
                for (int d = 0; d < 8; d++)
                {
                    int step = 1;
                    while (1)
                    {
                        int newX = i + step * directions[d][0];
                        int newY = j + step * directions[d][1];
                        if (newX < 0 || newX >= 8 || newY < 0 || newY >= 8)
                        {
                            break;
                        }
                        if (board[newX][newY] == ' ')
                        {
                            step++;
                            continue;
                        }
                        if (isOpponentPiece(board[newX][newY], currentPlayer))
                        {
                            addCaptureMove(&moves, count, board, i, j, newX, newY);
                        }
                        break;
                    }
                }
            }
            break;

                // ------------------------ King moves ------------------------

            case 'K': // white King
            case 'k': // black King
            {
                int directions[8][2] = {
                    {-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, {0, 1}, {1, -1}, {1, 0}, {1, 1}};
                for (int d = 0; d < 8; d++)
                {
                    int newX = i + directions[d][0];
                    int newY = j + directions[d][1];
                    if (newX >= 0 && newX < 8 && newY >= 0 && newY < 8)
                    {
                        if (isOpponentPiece(board[newX][newY], currentPlayer))
                        {
                            addCaptureMove(&moves, count, board, i, j, newX, newY);
                        }
                    }
                }
            }
            break;

            default:
                break;
            }
        }
    }

    return moves;
}

// --- QUIESCENCE SEARCH ---

long long quiesce(Node *node, long long alpha, long long beta, char currentPlayer)
{
    if (time_is_up()) {
        // If time is up, return a simple evaluation.
        return evaluateBoard(node->board);
    }
    long long stand_pat = evaluateBoard(node->board);
    if (currentPlayer == 'w')
    {
        if (stand_pat >= beta)
            return beta;
        if (stand_pat > alpha)
            alpha = stand_pat;
    }
    else
    { // currentPlayer == 'b'
        if (stand_pat <= alpha)
            return alpha;
        if (stand_pat < beta)
            beta = stand_pat;
    }
    // generating capture moves for current position
    int captureCount = 0;
    CaptureNode *captureMoves = generateCaptureMoves(node->board, currentPlayer, &captureCount);

    // recursive evaluation of  capture moves with quiescence search
    for (int i = 0; i < captureCount; i++)
    {
        Node child;
        memcpy(child.board, captureMoves[i].newBoard, sizeof(child.board));
        long long score;

        // changing players for recursion
        if (currentPlayer == 'w')
            score = quiesce(&child, alpha, beta, 'b');
        else
            score = quiesce(&child, alpha, beta, 'w');

        if (currentPlayer == 'w')
        {
            if (score > alpha)
                alpha = score;
            if (alpha >= beta)
            {
                free(captureMoves);
                return beta; // β- cutoff
            }
        }
        else
        {
            if (score < beta)
                beta = score;
            if (beta <= alpha)
            {
                free(captureMoves);
                return alpha; // α- cutoff
            }
        }
    }
    free(captureMoves);
    return currentPlayer == 'w' ? alpha : beta;
}

// ------------------------- Minimax Algorithm ------------------------- //

long long minimax(Node *node, int depth, char currentPlayer, long long alpha, long long beta)
{
    if (time_is_up()) {
        return evaluateBoard(node->board);
    }
    unsigned long long key = computeHash(node->board, currentPlayer);
    long long ttEval;
    if (ttLookup(key, depth, &ttEval))
        return ttEval;

    // using quiesce when depth is 0 or when there are no avelable moves

    if (depth == 0 || node->childcount == 0)
    {
        long long eval = quiesce(node, alpha, beta, currentPlayer);
        ttStore(key, depth, eval);
        return eval;
    }

    if (node->childcount > 0)
    {
        qsort(node->children, node->childcount, sizeof(Node *), compareMoves);
    }

    if (currentPlayer == 'w')
    {
        long long maxEval = LLONG_MIN;
        for (int i = 0; i < node->childcount; i++)
        {
            long long eval = minimax(node->children[i], depth - 1, 'b', alpha, beta);
            if (eval > maxEval)
            {
                maxEval = eval;
            }
            if (maxEval > alpha)
                alpha = maxEval;
            if (beta <= alpha)
                break;
        }
        ttStore(key, depth, maxEval);
        return maxEval;
    }
    else
    {
        long long minEval = LLONG_MAX;
        for (int i = 0; i < node->childcount; i++)
        {
            long long eval = minimax(node->children[i], depth - 1, 'w', alpha, beta);
            if (eval < minEval)
            {
                minEval = eval;
            }
            if (minEval < beta)
                beta = minEval;
            if (beta <= alpha)
                break;
        }
        ttStore(key, depth, minEval);
        return minEval;
    }
}

Move findBestMove(Node *root, int depth, char currentPlayer)
{
    if (root->childcount == 0)
        return (Move){-1, -1, -1, -1 , 'A' , false , '\0'};

    long long bestEval = currentPlayer == 'w' ? LLONG_MIN : LLONG_MAX;
    int bestIndex = 0;

    for (int i = 0; i < root->childcount; i++)
    {
        long long eval = minimax(root->children[i], depth - 1, currentPlayer == 'w' ? 'b' : 'w', LLONG_MIN, LLONG_MAX);

        if ((currentPlayer == 'w' && eval > bestEval) || (currentPlayer == 'b' && eval < bestEval))
        {
            bestEval = eval;
            bestIndex = i;
        }
    }
    return root->children[bestIndex]->move;
}


Move iterativeDeepening(Node *root, double timelimit, char currentPlayer) {
    Move bestMove = {-1, -1, -1, -1, 'A' , false , '\0'};
    struct timespec start, current;
    // Get the start time.
    clock_gettime(CLOCK_MONOTONIC, &start);
    // Set the global deadline.
    search_deadline.tv_sec = start.tv_sec + (int)timelimit;
    search_deadline.tv_nsec = start.tv_nsec + (long)((timelimit - (int)timelimit) * 1e9);
    if (search_deadline.tv_nsec >= 1e9) {
        search_deadline.tv_sec += 1;
        search_deadline.tv_nsec -= 1e9;
    }
    
    int depth = 1;
    while (true) {
        clock_gettime(CLOCK_MONOTONIC, &current);
        double elapsed = (current.tv_sec - start.tv_sec) +
                            (current.tv_nsec - start.tv_nsec) / 1e9;
        if (elapsed >= timelimit) {
            break; // Stop if the time limit is reached.
        }
        // Perform a depth-limited search at the current depth.
        Move currentMove = findBestMove(root, depth, currentPlayer);
        bestMove = currentMove;  // Save the best move from the last completed depth.
        depth++;  // Increase search depth.
    }
    return bestMove;
}

// ------------------------- Main Function ------------------------- //
// int main(int argc, char *argv[]) {
char *find_best_move(FEN fen, char **moves, int move_count, int time_limit)
{

    // Create root node
    Node *root = createNode(fen.board, (Move){-1, -1, -1, -1, 'A' , false , '\0'});

    // Process candidate moves
    for (int i = 0; i < move_count; i++)
    {
        Move move;
        if (decodeMove(root->board, moves[i], &move, fen.turn))
        {
            char newBoard[8][8];
            applyMove(root->board, newBoard, move);
            Node *child = createNode(newBoard, move);
            addChild(root, child);
        }
    }

    if (root->childcount == 0)
    {
        fprintf(stderr, "No valid moves provided\n");
        freeTree(root);
        return NULL;
    }

    // Find and print best move
    initTT();
    Move bestMove = iterativeDeepening(root, time_limit, fen.turn);
    char *algMove = malloc(6 * sizeof(char));
    moveToAlgebraicString(bestMove, algMove);
    freeTree(root);
    // printf("%s\n", algMove);
    return algMove;
    // return EXIT_SUCCESS;
}
