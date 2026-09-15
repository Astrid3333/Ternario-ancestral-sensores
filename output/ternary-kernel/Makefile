# Makefile para Kernel Ternario Ancestral v2
# Compilación para i386 (x86) usando gcc estándar con -m32

CC = gcc
AS = nasm
LD = ld
OBJCOPY = objcopy

CFLAGS = -ffreestanding -O2 -Wall -Wextra -nostdlib -nostdinc -fno-builtin \
         -fno-stack-protector -nostartfiles -nodefaultlibs -m32 -fno-pie -no-pie
ASFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T linker.ld -no-pie -nostdlib

BOOT_SRC = boot/boot.asm
BOOT_OBJ = boot/boot.bin

KERNEL_SRC = src/kernel.c src/memory.c src/scheduler.c
KERNEL_OBJ = kernel.o memory.o scheduler.o

KERNEL_BIN = ternary_kernel.bin
ISO_FILE = ternary_os.iso

all: $(ISO_FILE)

$(BOOT_OBJ): $(BOOT_SRC)
	$(AS) -f bin $< -o $@

kernel.o: src/kernel.c include/ternary.h
	$(CC) $(CFLAGS) -c src/kernel.c -o kernel.o

memory.o: src/memory.c include/ternary.h
	$(CC) $(CFLAGS) -c src/memory.c -o memory.o

scheduler.o: src/scheduler.c include/ternary.h
	$(CC) $(CFLAGS) -c src/scheduler.c -o scheduler.o

linker.ld:
	@echo 'OUTPUT_FORMAT("binary")' > linker.ld
	@echo 'ENTRY(kernel_main)' >> linker.ld
	@echo 'SECTIONS' >> linker.ld
	@echo '{' >> linker.ld
	@echo '  . = 0x10000;' >> linker.ld
	@echo '  .text : { *(.text) }' >> linker.ld
	@echo '  .rodata : { *(.rodata) }' >> linker.ld
	@echo '  .data : { *(.data) }' >> linker.ld
	@echo '  .bss : { *(.bss) }' >> linker.ld
	@echo '}' >> linker.ld

$(KERNEL_BIN): $(KERNEL_OBJ) linker.ld
	$(LD) $(LDFLAGS) $(KERNEL_OBJ) -o $@

disk.img: $(BOOT_OBJ) $(KERNEL_BIN)
	dd if=/dev/zero of=$@ bs=512 count=20
	dd if=$(BOOT_OBJ) of=$@ bs=512 count=1 conv=notrunc
	dd if=$(KERNEL_BIN) of=$@ bs=512 seek=1 conv=notrunc

$(ISO_FILE): disk.img
	mkdir -p iso/boot/grub
	cp disk.img iso/boot/ternary.img
	@echo 'set timeout=0' > iso/boot/grub/grub.cfg
	@echo 'set default=0' >> iso/boot/grub/grub.cfg
	@echo '' >> iso/boot/grub/grub.cfg
	@echo 'menuentry "Ternary Ancestral Kernel v2" {' >> iso/boot/grub/grub.cfg
	@echo '  linux16 /boot/ternary.img' >> iso/boot/grub/grub.cfg
	@echo '}' >> iso/boot/grub/grub.cfg
	genisoimage -R -b boot/grub/grub.cfg -no-emul-boot -o $@ iso/

run: $(ISO_FILE)
	qemu-system-i386 -cdrom $(ISO_FILE) -m 4M -serial stdio

debug: $(ISO_FILE)
	qemu-system-i386 -cdrom $(ISO_FILE) -m 4M -serial stdio -s -S &
	gdb -ex "target remote :1234"

run-disk: disk.img
	qemu-system-i386 -drive file=disk.img,format=raw -m 4M -serial stdio

clean:
	rm -f $(BOOT_OBJ) $(KERNEL_OBJ) $(KERNEL_BIN) disk.img $(ISO_FILE)
	rm -f linker.ld
	rm -rf iso/

info: $(KERNEL_BIN)
	@echo "=== Ternary Ancestral Kernel v2 ==="
	@echo "Boot loader: $(shell wc -c < $(BOOT_OBJ)) bytes"
	@echo "Kernel:      $(shell wc -c < $(KERNEL_BIN)) bytes"
	@echo "Total:       $(shell expr $(shell wc -c < $(BOOT_OBJ)) + $(shell wc -c < $(KERNEL_BIN))) bytes"
	@echo ""
	@echo "Features:"
	@echo "  - Nivel 2: Terminal + Shell interactivo"
	@echo "  - VGA con scroll y colores"
	@echo "  - Keyboard driver (scancode set 1)"
	@echo "  - Serial logging (9600 baud)"
	@echo "  - Ternary logic {-1, 0, +1}"
	@echo "  - Memory: Base 60 (Babylonian)"
	@echo "  - Scheduler: Maya cycles"
	@echo "  - Filesystem: Quipu"
	@echo ""
	@echo "Shell commands:"
	@echo "  help, ps, mem, fs, cal, fork, kill, echo, clear, trit, b60, halt"

.PHONY: all clean run debug run-disk info
