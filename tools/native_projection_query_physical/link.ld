ENTRY(_start)
PHDRS { image PT_LOAD FLAGS(7); heap PT_LOAD FLAGS(6); stk PT_LOAD FLAGS(6); rstack PT_LOAD FLAGS(6); runner PT_LOAD FLAGS(7); }
SECTIONS {
  .image 0x12000 : { *(.image) } :image
  .heap 0x20000000 (NOLOAD) : { . += 0x100000; } :heap
  .stk 0x30000000 (NOLOAD) : { . += 0x40000; } :stk
  .rstack 0x6fd00000 (NOLOAD) : { . += 0x200000; } :rstack
  .runner 0x70000000 : { *(.runner_text) *(.text*) *(.runner_data) *(.data*) *(.rodata*) *(.bss*) *(COMMON) } :runner
  /DISCARD/ : { *(.note*) *(.comment) *(.eh_frame*) }
}
