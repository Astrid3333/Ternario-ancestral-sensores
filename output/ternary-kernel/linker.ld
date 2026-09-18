OUTPUT_FORMAT("elf32-i386")
ENTRY(kernel_main)
SECTIONS
{
  . = 0x100000;  /* 1MB — standard for protected mode kernels */
  .multiboot : { *(.multiboot) }
  .text : { *(.text) }
  .rodata : { *(.rodata*) }
  .data : { *(.data) }
  .bss : { *(.bss) }
}
