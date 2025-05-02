## Short makefile to compile my C chess engine

## Where our implementation is located (don't change)
SRCDIR = src
INCDIR = $(SRCDIR)/include

## List all files you want to be part of your engine below
## (Okay to add more files)
SOURCES = \
  $(SRCDIR)/main.c

SOURCES_INC = \
  $(INCDIR)/fen.c \
  $(INCDIR)/boardmap.c \
  $(INCDIR)/find_best_move.c \
  $(INCDIR)/find_moves.c

## You SHOULD NOT modify the parameters below

## Compiler to use by default
CC = gcc

## Compiler flags
CFLAGS = -I$(INCDIR)/Headers -Wall -Wextra -Werror -pedantic

## Where to put the object files
BINDIR ?= build

## Object filenames derived from source filenames
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(BINDIR)/%.o) \
          $(SOURCES_INC:$(INCDIR)/%.c=$(BINDIR)/%.o)

# The name of the binary (executable) file
TARGET ?= main

## Optional target: useful for running the website
WEB_TARGET ?= main.wasm

## Emscripten compiler
EMCC = emcc

## Emscripten flags
EMCC_FLAGS = -s WASM=1 -s EXPORTED_FUNCTIONS='["_choose_move"]' --no-entry -O3

## Create the build directory if it doesn't exist
$(BINDIR):
	mkdir -p $(BINDIR)

## Compile each object file (ensure the object is placed in the right directory)
$(BINDIR)/%.o: $(SRCDIR)/%.c $(BINDIR)
	$(CC) $(CFLAGS) -c $< -o $@

## Add a rule for the files in the include directory
$(BINDIR)/%.o: $(INCDIR)/%.c $(BINDIR)
	$(CC) $(CFLAGS) -c $< -o $@

## Compile the final binary
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@

## Only build the binary by default
all: $(TARGET)

## Optional target: build the web target
$(WEB_TARGET): $(SOURCES) $(SOURCES_INC)
	emcc $(EMCC_FLAGS) $^ -o $@

## Start python3 web server to run the website for the folder web
.PHONY: run
run: $(WEB_TARGET)
	python3 -m http.server --directory web

## Clean up the build directory
.PHONY: clean
clean:
	rm -rf $(BINDIR) $(TARGET) $(WEB_TARGET)
