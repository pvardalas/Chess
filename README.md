# Setup:
```sh
make main
```

## Wanna see it run on a browser? Currently facing some issues :/
## You can still try it out on terminal. After compiling just type:
```sh
./main
```
### For when the browser version comes back up:
First compile it to webassembly - requires the emscripten compiler (`apt install emscripten` if you don't have it) and then run:

```sh
make WEB_TARGET=web/engine.wasm web/engine.wasm
```

Finally, run the website (assumes you have python3 installed):

```sh
$ make run
python3 -m http.server --directory web
Serving HTTP on 0.0.0.0 port 8000 (http://0.0.0.0:8000/) ...
```

You should now be able to interact with your engine on http://0.0.0.0:8000/.

# Project Explaination:

#  boardmap
## boardmap.h
This header file is the fundation for “`boardmap.c `”, where the chess board is represented by bitboards. In more detail in this file we set up bitboard operations , declare global variables and  essential functions.


## boardmap.c

First we decare the bitboards for white pieces (`U64 whitePieces;`) , for black pieces (`U64 blackPieces;`) , and for every piece in the board (`U64 pieces;`). ( Here “`U64`” is defined in boardmap.h ).  Then we initialize them in the function `initialize_board `


```sh
void initialize_board() { 
whitePieces = 0x000000000000FFFFULL; 
blackPieces = 0xFFFF000000000000ULL; 
pieces = whitePieces | blackPieces; 
}
```
Then , depending on whose turn it is (black or white) we return the appropreate in the function `getGlobalVar`:
```sh
U64 getGlobalVar(char color) {
    if (color == 'w'){
        return whitePieces;
    }else if (color == 'b'){
        return blackPieces;
    }else{
        return pieces;
    }
}
```

# Το engine δεν είναι τελειωμένο. Έχει γίνει αρκετή δουλειά, παρ'όλα αυτά το project θα ολοκληρωθεί εκπρόθεσμα. Για την ώρα δεν είναι απόλυτα λειτουργικό.


## fen.h
This header file is about chess positions using FEN. It contains the definition of the FEN structure, function declarations (`parse_fen`,`new_fen`,`coord_to_square`)(that will be used in fen.c)

## fen.c

### `parce_fen` function
In this function we first have to initialize the board with spaces. First we have to tokenize the FEN string by spaces. The first token represents the board layout, using digits to indicate consecutive empty squares and slashes `/` to separate rows. The FEN also tells us the turn, castling rights, en passant targets, half-move clock and total moves.

### `generate_fen` function 
This function constructs a FEN string from a given FEN structure . it iterates over each row and counts empty spaces, adds the piece symbols or the number of empty squares and adds information about whose turn it is , castling rights, en passant target, half-move clock and full move number.

### `coord_to_square` function
This function converts board array coords into standard algebraic notation.
### `is_enpassant_square` function
This function takes the coordinate conversion to check if a given space matches the en passant target stored in the FEN structure.

#
### `is_valid_move_pawn` function
Checks single and double forward moves, diagonal captures and en passant captures (for pawns)
#
### `is_valid_move_knight` function
Checks for L-shaped based moves (for knights)
#
### `is_valid_move_bishop`function
Checks diagonal moves, ensuring no other  pieces block the path (for bishops)
#
### `is_valid_move_rook` function
Checks horizontal or vertical moves ,and that that the path is clear (for rooks)
#
### `is_valid_move_queen` function
Combines the logic of both bishop and rook moves (for queen)
#
### `is_valid_move_king` function 
Checks that the king moves only one square in any direction (for king)
#
### ` is_valid_move` function 
In addition to all the piece specified rules , this function verifies that the destination square is not occupied by a friendly piece 

#
### ` apply_move` function 
Takes the move expressed in algebraic notation and updates the board accordingly. In more detail, for castling it moves both king and rook to their new positions, and also removes the rightd for the moving side. The function finds what to move from the notation (it identifies the destination square and piece). For pawns it identifies special cases such as double moves, diagonal captures, en passant and promotions. (it promotes to a queen by default if no promotion piece is specified) . if no piece can legally move to the destination square it  prints an error and exits. Then the bitboard is updated using ` SET_BIT` , ` CLEAR_BIT` previously declared in  `boardmap.h`. the half-move clock resets if a pawn is moves otherwise it increments, if a king or a rook are moved then it updates the castling rights. At the end of the move, the  turn is switched to the opposite color 

### ` new_fen` function 
This function takes the initial FEN structure and a move string , applies the move using `apply_move` function and returns the updated board position as a new FEN structure

#
#
# find_moves
## find_moves.h
This header file is for  move generation (legal chess moves according to chess rules) using bitboards. This declares the functions `find_moves` , which will be used to find all legal moves, `initNonSlidingMoves` which are precomputed moves for knights, kings, and pawns , and `printBitboard` (debugging)


## find_moves.c
### `printBitboard` function
This function is used to display the 64 bit integer `bb` as an 8x8 grid (each bit represents a square on the chess board)
#
### `computeKnightAttacks`
In this function we calculate all possible knight move from a given square (it returns the computed bitboard that represents all legal moves for the knight from that square)
#
### `computeKingAttacks` function 
 In this we calculate all possible moves for a king from a given square (it returns the bitboard of all squares that the king can move to from a given square)
#
### `initNonSlidingMoves` function 
This function initializes a global array by computing and storing all possible moves for a knight and king for every square on the board 
#
### `find_moves` function 
This function generates all legal moves for the player whose turn it is  using bitboards to be more efficient .


#
#
# move
## move.h
This header  declares the `move ` function wich is responsible for updating the game state based on a given move
## move.c
### `moves_count ` Function
This function counts the number of moves that are represented in a string (where each move is separated by a space).
#
### `move` Function
This function counts the moves provides in the moves string


#
#
# find_best_move
## find_best_move.h
This header declares the function `find_best_move`

## find_best_move.c

### structures
First thig we do is define the value of each piece. This will be used later when evaluating the board. Then we create the structure “`Move`” , which holds the information of a move. Meaning from what square the piece started , where the destination square is , the type of piece that moves, if the move is a capture, and if it is a promotion (for pawns ).
```sh
typedef struct {
    int fromX, fromY, toX, toY;
    char piece;         
    bool isCapture;     
    char promotion; 
} Move;
``` 

Then we create the structure of the tree nodes “`Node`” . this Node holds the current state of the game (the current board ) , the move that lead to this node, the evaluation of the move (`moveScore`), an array of pointers to child nodes (`children`), and the amount of children the tree has (`childcount`)
```sh
typedef struct node {
    char board[8][8];
    Move move;
    long long int moveScore;
    struct node **children;
    int childcount;
} Node;
```
Then we create the structure of the transposition table entry (`TTEntry`).This structure is used to cache previously computed board evaluations to speed up search (basically avoid re-exloration).
```sh
typedef struct {
    unsigned long long key;
    long long evaluation;
    int depth;
} TTEntry;
```

### `decodeMove` function 
This function takes the move in short algebraic notation and “decodes it”. What I  mean by this is it takes the string and extractes all the information inorder to use it later.it checks for castling also, and primotions.it extracts the destination square from the last 2 characters . if the first character is one of  "K Q R B N" then it means it is not a pawn and we need to save what piece it is. For pawns it distinguishesbetween simple forward moves and captures.

#
### `moveToAlgebraicString` function 
This function builds an algebraic string representation from a `Move` structure 
(it takes all the information (if it is a castle , capture , promotion , destination square)).


#
### `evaluateBoard` function 
This function iterates over every square of the board, summing up the values of white pieces and subtracting the values of black pieces.
#
### `applyMove` function 
This function creates new board states by copying the source board, moving the piece from its starting square to the destination (playing the move ), and then emptying the original square.

### `createNode` 
This function creates a new node to the game tree (allocates and initializes a new node). 
#
### `addChild` function
It dynamically resizes the parents children array (using realloc) and adds a new child to the tree (basically adds a new possible state of the game )
#
### `freeTree` function 
This function frees all the allocated memomy used by the tree.


#
### `computeHash`
This algorithm helps quickly compare board states, making the program more efficient 

#
### `generateCaptureMoves` function
This function scans the board for moves that capture enemy pieces

#
### `quiesce` function
This function first computes an evaluation of the current board, then generates all capture moves and recursively evaluates these moves while applying alpha–beta cutoffs.
# 
### `minimax` function
This is the core recursive search algorithm. It choses the bmove with the highest board evaluation 


#
### ` iterativeDeepening``function 
This function instead of searching to fixed depth starts with a shallow depth and incrementally increases it. This method allows the engine to return the best move found within the allotted time


# main
## main.c
Here all the other filles work together to create the chess engine.
### header - libraries
 ```sh
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include "include/Headers/fen.h"
 #include "include/Headers/find_moves.h" 
#include "include/Headers/boardmap.h" 
#include "include/Headers/find_best_move.h"
```

Here we include all the necessary libraries, as well as all the headers we created.

#
### `print_fen` function
This function takes a pointer to a FEN structure and prints the current board state

#
### `get_input` function 
This function allocates a new string (512 characters) and copies the input string into it while omitting any double-quote characters. It ensures the returned string is null-terminated

#
### `IndexMove` function 
This function takes a string of moves and an index and extracts the move at that index 
#
### `move_idx` function
This function searches for a specific move within the moves string and returns its index
 #
### `print_fen_board` function
This is a helper function that prints the board as an 8×8 grid, displaying each square’s content with tabs for readability
#
### `print_bit` function
Prints a bitboard representation (using a 64-bit unsigned integer). It checks each bit (using a macro GET_BIT) and prints a "1" if the bit is set or a dot otherwise

#
### `choose_move` function
This is the core function of the programs decision making.

#
### `main` function

First we have to check if there are exactly 3 arguments given (FEN string, moves string, and timeout value). Then it calls the functions `initialize_board` , `initNonSlidingMoves`and `choose_move` to connect with the other parts of the program and function.





