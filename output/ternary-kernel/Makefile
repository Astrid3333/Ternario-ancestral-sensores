# Makefile para Kernel Ternario Ancestral v3
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

MB_HEADER_SRC = boot/multiboot_header.asm
MB_HEADER_OBJ = boot/multiboot_header.o

KERNEL_SRC = src/kernel.c src/memory.c src/scheduler.c src/net.c src/fs.c src/tls.c \
             src/pkg.c src/firewall.c src/dl.c src/mail.c src/chat.c \
             src/vpn.c src/doh.c src/block.c src/parental.c src/ai_client.c \
             src/ftp.c src/sockets.c src/gdt.c src/paging.c src/syscalls.c \
             src/elf.c src/libc.c src/framebuffer.c src/mouse.c \
             src/shell_user.c src/libc_full.c src/window.c \
             src/usb_storage.c src/elf_dynamic.c src/user.c
KERNEL_OBJ = boot/start.o boot/multiboot_header.o boot/gdt_flush.o boot/context_switch.o kernel.o memory.o scheduler.o net.o fs.o tls.o \
             pkg.o firewall.o dl.o mail.o chat.o \
             vpn.o doh.o block.o parental.o ai_client.o \
             ftp.o sockets.o gdt.o paging.o syscalls.o \
             elf.o libc_full.o framebuffer.o mouse.o \
              shell_user.o window.o process.o usb.o ac97.o edu.o tri.o phase3.o phase4.o apps.o fonts.o ternary_ops.o tric.o math_shell.o tutorials.o lab.o \
              usb_storage.o elf_dynamic.o user.o

KERNEL_ELF = ternary_kernel.elf
KERNEL_BIN = ternary_kernel.bin
DISK_IMG = disk.img
ISO_FILE = ternary_os.iso

all: $(ISO_FILE) $(DISK_IMG)

$(BOOT_OBJ): $(BOOT_SRC)
	$(AS) -f bin $< -o $@

$(MB_HEADER_OBJ): $(MB_HEADER_SRC)
	$(AS) -f elf32 $< -o $@

boot/gdt_flush.o: boot/gdt_flush.asm
	$(AS) -f elf32 $< -o $@

boot/context_switch.o: boot/context_switch.asm
	$(AS) -f elf32 $< -o $@

boot/start.o: boot/start.asm
	$(AS) -f elf32 $< -o $@

kernel.o: src/kernel.c include/ternary.h
	$(CC) $(CFLAGS) -c src/kernel.c -o kernel.o

memory.o: src/memory.c include/ternary.h
	$(CC) $(CFLAGS) -c src/memory.c -o memory.o

scheduler.o: src/scheduler.c include/ternary.h
	$(CC) $(CFLAGS) -c src/scheduler.c -o scheduler.o

net.o: src/net.c include/ternary.h
	$(CC) $(CFLAGS) -c src/net.c -o net.o

fs.o: src/fs.c include/ternary.h
	$(CC) $(CFLAGS) -c src/fs.c -o fs.o

tls.o: src/tls.c include/ternary.h
	$(CC) $(CFLAGS) -c src/tls.c -o tls.o

pkg.o: src/pkg.c include/ternary.h
	$(CC) $(CFLAGS) -c src/pkg.c -o pkg.o

firewall.o: src/firewall.c include/ternary.h
	$(CC) $(CFLAGS) -c src/firewall.c -o firewall.o

dl.o: src/dl.c include/ternary.h
	$(CC) $(CFLAGS) -c src/dl.c -o dl.o

mail.o: src/mail.c include/ternary.h
	$(CC) $(CFLAGS) -c src/mail.c -o mail.o

chat.o: src/chat.c include/ternary.h
	$(CC) $(CFLAGS) -c src/chat.c -o chat.o

vpn.o: src/vpn.c include/ternary.h
	$(CC) $(CFLAGS) -c src/vpn.c -o vpn.o

doh.o: src/doh.c include/ternary.h
	$(CC) $(CFLAGS) -c src/doh.c -o doh.o

block.o: src/block.c include/ternary.h
	$(CC) $(CFLAGS) -c src/block.c -o block.o

parental.o: src/parental.c include/ternary.h
	$(CC) $(CFLAGS) -c src/parental.c -o parental.o

ai_client.o: src/ai_client.c include/ternary.h
	$(CC) $(CFLAGS) -c src/ai_client.c -o ai_client.o

ftp.o: src/ftp.c include/ternary.h
	$(CC) $(CFLAGS) -c src/ftp.c -o ftp.o

sockets.o: src/sockets.c include/ternary.h
	$(CC) $(CFLAGS) -c src/sockets.c -o sockets.o

gdt.o: src/gdt.c include/ternary.h
	$(CC) $(CFLAGS) -c src/gdt.c -o gdt.o

paging.o: src/paging.c include/ternary.h
	$(CC) $(CFLAGS) -c src/paging.c -o paging.o

syscalls.o: src/syscalls.c include/ternary.h
	$(CC) $(CFLAGS) -c src/syscalls.c -o syscalls.o

elf.o: src/elf.c include/ternary.h
	$(CC) $(CFLAGS) -c src/elf.c -o elf.o

libc.o: src/libc.c include/ternary.h
	$(CC) $(CFLAGS) -c src/libc.c -o libc.o

framebuffer.o: src/framebuffer.c include/ternary.h
	$(CC) $(CFLAGS) -c src/framebuffer.c -o framebuffer.o

mouse.o: src/mouse.c include/ternary.h
	$(CC) $(CFLAGS) -c src/mouse.c -o mouse.o

shell_user.o: src/shell_user.c include/ternary.h
	$(CC) $(CFLAGS) -c src/shell_user.c -o shell_user.o

libc_full.o: src/libc_full.c include/ternary.h
	$(CC) $(CFLAGS) -c src/libc_full.c -o libc_full.o

window.o: src/window.c include/ternary.h
	$(CC) $(CFLAGS) -c src/window.c -o window.o

process.o: src/process.c include/ternary.h
	$(CC) $(CFLAGS) -c src/process.c -o process.o

usb.o: src/usb.c include/ternary.h
	$(CC) $(CFLAGS) -c src/usb.c -o usb.o

ac97.o: src/ac97.c include/ternary.h
	$(CC) $(CFLAGS) -c src/ac97.c -o ac97.o

edu.o: src/edu.c include/ternary.h
	$(CC) $(CFLAGS) -c src/edu.c -o edu.o

tri.o: src/tri.c include/ternary.h
	$(CC) $(CFLAGS) -c src/tri.c -o tri.o

phase3.o: src/phase3.c include/ternary.h
	$(CC) $(CFLAGS) -c src/phase3.c -o phase3.o

phase4.o: src/phase4.c include/ternary.h
	$(CC) $(CFLAGS) -c src/phase4.c -o phase4.o

apps.o: src/apps.c include/ternary.h
	$(CC) $(CFLAGS) -c src/apps.c -o apps.o

fonts.o: src/fonts.c include/ternary.h
	$(CC) $(CFLAGS) -c src/fonts.c -o fonts.o

ternary_ops.o: src/ternary_ops.c include/ternary.h
	$(CC) $(CFLAGS) -c src/ternary_ops.c -o ternary_ops.o

tric.o: src/tric.c include/ternary.h
	$(CC) $(CFLAGS) -c src/tric.c -o tric.o

math_shell.o: src/math_shell.c include/ternary.h
	$(CC) $(CFLAGS) -c src/math_shell.c -o math_shell.o

tutorials.o: src/tutorials.c include/ternary.h
	$(CC) $(CFLAGS) -c src/tutorials.c -o tutorials.o

lab.o: src/lab.c include/ternary.h
	$(CC) $(CFLAGS) -c src/lab.c -o lab.o

usb_storage.o: src/usb_storage.c include/ternary.h
	$(CC) $(CFLAGS) -c src/usb_storage.c -o usb_storage.o

elf_dynamic.o: src/elf_dynamic.c include/ternary.h
	$(CC) $(CFLAGS) -c src/elf_dynamic.c -o elf_dynamic.o

user.o: src/user.c include/ternary.h
	$(CC) $(CFLAGS) -c src/user.c -o user.o

$(KERNEL_ELF): $(MB_HEADER_OBJ) $(KERNEL_OBJ)
	$(LD) $(LDFLAGS) -o $@ $^

$(KERNEL_BIN): $(KERNEL_ELF)
	$(OBJCOPY) -O binary $< $@

$(DISK_IMG): $(BOOT_OBJ) $(KERNEL_BIN)
	dd if=/dev/zero of=$@ bs=512 count=40 2>/dev/null
	dd if=$(BOOT_OBJ) of=$@ bs=512 count=1 conv=notrunc 2>/dev/null
	dd if=$(KERNEL_BIN) of=$@ bs=512 seek=1 conv=notrunc 2>/dev/null

$(ISO_FILE): $(KERNEL_ELF)
	mkdir -p iso/boot/grub
	cp $(KERNEL_ELF) iso/boot/ternary_kernel.elf
	@echo 'set timeout=0' > iso/boot/grub/grub.cfg
	@echo 'set default=0' >> iso/boot/grub/grub.cfg
	@echo '' >> iso/boot/grub/grub.cfg
	@echo 'menuentry "Ternary Ancestral Kernel v3 (Network)" {' >> iso/boot/grub/grub.cfg
	@echo '    multiboot /boot/ternary_kernel.elf' >> iso/boot/grub/grub.cfg
	@echo '    boot' >> iso/boot/grub/grub.cfg
	@echo '}' >> iso/boot/grub/grub.cfg
	grub-mkrescue -o $@ iso/ 2>&1 || genisoimage -R -b boot/grub/grub.cfg -no-emul-boot -o $@ iso/

run: $(ISO_FILE)
	qemu-system-i386 -cdrom $(ISO_FILE) -m 4M -serial stdio -device rtl8139,netdev=net0 -netdev user,id=net0

debug: $(ISO_FILE)
	qemu-system-i386 -cdrom $(ISO_FILE) -m 4M -serial stdio -s -S -device rtl8139,netdev=net0 -netdev user,id=net0 &
	gdb -ex "target remote :1234"

run-disk: $(DISK_IMG)
	qemu-system-i386 -drive file=$(DISK_IMG),format=raw -m 4M -serial stdio -device rtl8139,netdev=net0 -netdev user,id=net0

clean:
	rm -f $(BOOT_OBJ) $(KERNEL_OBJ) $(KERNEL_ELF) $(KERNEL_BIN) $(DISK_IMG) $(ISO_FILE)
	rm -rf iso/

info: $(KERNEL_BIN)
	@echo "=== Ternary Ancestral Kernel v3 ==="
	@echo "Boot loader: $(shell wc -c < $(BOOT_OBJ)) bytes"
	@echo "Kernel ELF:  $(shell wc -c < $(KERNEL_ELF)) bytes"
	@echo "Kernel bin:  $(shell wc -c < $(KERNEL_BIN)) bytes"
	@echo "Total:       $(shell expr $(shell wc -c < $(BOOT_OBJ)) + $(shell wc -c < $(KERNEL_BIN))) bytes"

.PHONY: all clean run debug run-disk info
