# Makefile for Tritos - Ternary Ancestral Kernel (Userspace)
# Compiles all modules: shell, science, AI, browser, terminal

CC = gcc
CFLAGS = -O2 -Wall -Wextra -std=c99 -D_XOPEN_SOURCE=500 -Iinclude
LDFLAGS = -lm -lz -lbz2 -llzma

# Core shell
SHELL_SRC = src/tak.c src/scheduler.c src/filesystem.c src/memory.c
SHELL_BIN = tritos

# Science suite
SCIENCE_SRC = src/tritos_science.c src/ising_ternary.c src/lga_ternary.c src/md_ternary.c \
              src/sensor_agro_ternary.c src/sensor_environment_ternary.c src/sensor_seismic_ternary.c \
              src/hopfield_ternary.c src/perceptron_ternary.c src/blender_ternary.c \
              src/compression_ternary.c src/render_ternary.c src/stl_writer.c \
              src/audio_ternary.c src/crypto_ternary.c src/ternary_logic.c \
              src/compress_optimized.c
SCIENCE_BIN = tritos_science

# AI engine
AI_SRC = src/ternary_ai.c src/ternary_logic.c
AI_BIN = tritos_ai

# TUI browser
BROWSER_SRC = src/tui_browser.c
BROWSER_BIN = tritos_browser

# Terminal calculator
TERM_SRC = src/ternary_terminal.c
TERM_BIN = tritos_terminal

# File compressor
COMPRESS_SRC = src/ternary_filecompress.c
COMPRESS_BIN = tritos_compress

# GUI
GUI_SRC = tritos_gui.py
GUI_BIN = tritos-gui

# Encyclopedia
ENCY_SRC = tritos_encyclopedia.py
ENCY_BIN = tritos_encyclopedia.py

# All targets
all: $(SHELL_BIN) $(SCIENCE_BIN) $(AI_BIN) $(BROWSER_BIN) $(TERM_BIN) $(COMPRESS_BIN)
	@echo ""
	@echo "  ╔══════════════════════════════════════════╗"
	@echo "  ║   TRITOS — Todos los módulos compilados  ║"
	@echo "  ╠══════════════════════════════════════════╣"
	@echo "  ║  Shell:     $(shell du -h $(SHELL_BIN) | cut -f1)                        ║"
	@echo "  ║  Science:   $(shell du -h $(SCIENCE_BIN) | cut -f1)                        ║"
	@echo "  ║  AI:        $(shell du -h $(AI_BIN) | cut -f1)                        ║"
	@echo "  ║  Browser:   $(shell du -h $(BROWSER_BIN) | cut -f1)                        ║"
	@echo "  ║  Terminal:  $(shell du -h $(TERM_BIN) | cut -f1)                        ║"
	@echo "  ║  Compress:  $(shell du -h $(COMPRESS_BIN) | cut -f1)                        ║"
	@echo "  ╚══════════════════════════════════════════╝"
	@echo ""

# Shell
$(SHELL_BIN): $(SHELL_SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "  ✓ Built: $(SHELL_BIN)"

# Science
$(SCIENCE_BIN): $(SCIENCE_SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "  ✓ Built: $(SCIENCE_BIN)"

# AI
$(AI_BIN): $(AI_SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "  ✓ Built: $(AI_BIN)"

# Browser
$(BROWSER_BIN): $(BROWSER_SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "  ✓ Built: $(BROWSER_BIN)"

# Terminal
$(TERM_BIN): $(TERM_SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "  ✓ Built: $(TERM_BIN)"

# Compressor
$(COMPRESS_BIN): $(COMPRESS_SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "  ✓ Built: $(COMPRESS_BIN)"

# Clean
clean:
	rm -f $(SHELL_BIN) $(SCIENCE_BIN) $(AI_BIN) $(BROWSER_BIN) $(TERM_BIN) $(COMPRESS_BIN)
	rm -f src/*.o
	@echo "  ✓ Cleaned"

# Install
install: all
	install -m 755 $(SHELL_BIN) ~/.local/bin/tritos
	install -m 755 $(SCIENCE_BIN) ~/.local/bin/tritos_science
	install -m 755 $(AI_BIN) ~/.local/bin/tritos_ai
	install -m 755 $(BROWSER_BIN) ~/.local/bin/tritos_browser
	install -m 755 $(TERM_BIN) ~/.local/bin/tritos_terminal
	install -m 755 $(COMPRESS_BIN) ~/.local/bin/tritos_compress
	@echo "  ✓ Installed to ~/.local/bin/"

.PHONY: all clean install
