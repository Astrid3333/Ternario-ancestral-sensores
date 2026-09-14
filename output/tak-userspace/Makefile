# Makefile for Ternary Ancestral Kernel (Userspace)
# Compiles with regular gcc, runs on any Linux

CC = gcc
CFLAGS = -O2 -Wall -Wextra -std=c99 -D_XOPEN_SOURCE=500 -Iinclude
LDFLAGS = -lm

SRC = src/tak.c src/scheduler.c src/filesystem.c src/memory.c
OBJ = $(SRC:.c=.o)
BIN = tak

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo ""
	@echo "  ✓ Built: ./tak"
	@echo "  ✓ Run:   ./tak"
	@echo "  ✓ Size:  $$(du -h $(BIN) | cut -f1)"
	@echo ""

clean:
	rm -f $(BIN) $(OBJ)

install: $(BIN)
	install -m 755 $(BIN) /usr/local/bin/tak

.PHONY: all clean install
