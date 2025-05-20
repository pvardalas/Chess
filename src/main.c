#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/Headers/fen.h"
#include "include/Headers/find_moves.h"
#include "include/Headers/boardmap.h"
#include "include/Headers/find_best_move.h"
void print_fen(const FEN *board_fen) {
    for (int r = 0; r < 8; r++) {
        int emptyCount = 0;
        for (int c = 0; c < 8; c++) {
            char piece = board_fen->board[r][c];
            if (piece == ' ') {
                emptyCount++;
            } else {
                if (emptyCount > 0) {
                    printf("%d", emptyCount);
                    emptyCount = 0;
                }
                printf("%c", piece);
            }
        }
        if (emptyCount > 0) {
            printf("%d", emptyCount);
        }
        if (r != 7) printf("/");
    }

    printf(" %c", board_fen->turn);
    printf(" %s", (strlen(board_fen->castling) > 0) ? board_fen->castling : "-");
    printf(" %s", (strcmp(board_fen->enPassant, "-") == 0) ? "-" : board_fen->enPassant);
    printf(" %d %d\n", board_fen->halfMoveClock, board_fen->fullMoveNum);
}
char* get_input(const char* input) {
    if (!input) return NULL;

    int count = 0, count2 = 0;
    char *output = malloc(512 * sizeof(char));  // Allocate memory for the output string
    if (!output) return NULL;  // Check for malloc failure

    while (input[count2]) {
        if (input[count2] != '"') {
            output[count] = input[count2];  // Copy non-quote characters to output
            count++;
        }
        count2++;
    }
    if (count == 0) return NULL;
    output[count] = '\0';  // Null-terminate the string

    return output;  // Return the dynamically allocated string
}

char * IndexMove(char * moves, int index) {
    if (moves == NULL || index < 0) exit(EXIT_FAILURE);
    char * ptr = moves;
    char * result = malloc(6 * sizeof(char));
    char * start = result;
    int count = 1;
    while (*ptr) {
        if (*ptr == ' ') count++;
        if (count == index && *ptr != ' ') {
            *result = *ptr;
            result++;            
        }
        else if (count > index) break;
        ptr++;
    }
    if (count == index) return NULL;
    *result = '\0';
    return start;
    
}

int move_idx(char * moves, char * move) {
    int count = 0;  // Ensure count is an int
    int i = 0;

    while (moves[i] != '\0') {
        // Check if this position matches the move
        int j = 0;
        while (move[j] != '\0' && moves[i + j] == move[j]) {
            j++;
        }

        // If the full move matches and is followed by space or end of string
        if (move[j] == '\0' && (moves[i + j] == ' ' || moves[i + j] == '\0')) {
            return count;
        }

        // If space is found, increment move count
        if (moves[i] == ' ') {
            count++;
        }

        i++;  // Move to next character
    }

    return -1;  // Move not found
}
void print_fen_board(FEN fen) {

    printf("%s is playing\tHalf move clock: %d\tFull move clock: %d\n",(fen.turn == 'w') ? "White" : "Black", fen.halfMoveClock, fen.fullMoveNum);
    printf("\t═════◈═════════◈══════ ✮❁•° ♛ °•❁✮ ══════◈══════════◈═════\n");
    for (int i = 0; i < 8; i++) {
        printf("%d\t", 8-i);
        for (int j = 0; j < 8; j++) {
            printf("%c\t", fen.board[i][j]);
        }
        printf("\n");
    }
    printf("\t═════◈═════════◈══════◈═════════════◈═════◈══════════◈═════\n");
    printf("\ta\tb\tc\td\te\tf\tg\th\n");

}
void print_bit(U64 bb) {
    for (int rank = 7; rank >= 0; rank--) {
        for (int file = 0; file < 8; file++) {
            int sq = rank * 8 + file;
            if (GET_BIT(bb, sq)) printf("1");
            else printf(".");
        }
        printf("\n");
    }    
}

    



int main() {
    // if (argc != 4) {
    //     fprintf(stderr, "Usage: ./%s <fen> <moves> <timeout>\n", argv[0]);
    //     exit(EXIT_FAILURE);
    // }
    initialize_board();
    initNonSlidingMoves(); // Initializing the precomputed masks for every piece for later move generation
    FEN game; // Current game state
    parse_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq- 0 1", &game); // Initializing the game
    printf("Test\n");
    char * next_move = malloc(6 * sizeof(char));
    int n;
    char ** possible_moves = NULL;
    int possible_moves_count, flag = 0;
    while (1) {
        printf("\033[1;1H\033[2J");
        print_fen_board(game);
jump:
        printf("Next move: ");
        while((n = scanf("%5s", next_move)) != 1 && n != -1) {
            printf("Provide a valid move!\n");
            goto jump;
        }
        if (n == -1) {
            printf("Terminating!\n");
            break;
            exit(EXIT_SUCCESS);
        }
        find_moves(game, &possible_moves, &possible_moves_count);
        flag = 0;
        for (int i = 0; i < possible_moves_count; i++) {
            if (strcmp(next_move, possible_moves[i]) == 0) {
                flag = 1;
                break;
            }
        }
        if (!flag) {
            printf("Provide a valid move!\n");
            goto jump;
        }
        game = new_fen(game, next_move);

    }
}
