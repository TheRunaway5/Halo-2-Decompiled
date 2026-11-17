.code32
.section .runner_text,"ax"
.globl _start
_start:
    mov $0x6ff00000, %esp
    call runner_main
1:  jmp 1b
.globl call_original
call_original:
    push %ebp; push %ebx; push %esi; push %edi
    mov %esp, saved_esp
    fninit
    fldenv io_env
    ldmxcsr io_mxcsr
    mov io_esp, %ecx
    movl $back, (%ecx)
    mov io_regs, %ebx
    mov io_regs+4, %ebp
    mov io_regs+8, %esi
    mov io_regs+12, %edi
    mov io_entry, %eax
    mov %ecx, %esp
    jmp *%eax
back:
    movl $0, io_crashed
    mov %ebx, io_out
    mov %esp, io_out+4
    mov %ebp, io_out+8
    mov %esi, io_out+12
    mov %edi, io_out+16
    fnstsw io_out+20
    fnstcw io_out+24
    stmxcsr io_out+28
    mov saved_esp, %esp
    pop %edi; pop %esi; pop %ebx; pop %ebp
    ret
.globl crash_handler
crash_handler:
    movl $1, io_crashed
    mov saved_esp, %esp
    pop %edi; pop %esi; pop %ebx; pop %ebp
    ret
# Deterministic allocator object: vtable +10 allocate(bytes,class) RET 8,
# +14 release(data,bytes,class) RET 0xC. Bump top lives at [20000000].
alloc:
    mov 0x20000000, %eax
    mov 4(%esp), %edx
    add $15, %edx
    and $-16, %edx
    add %eax, %edx
    mov %edx, 0x20000000
    ret $8
release:
    ret $12
.section .runner_data,"aw"
.globl io_entry, io_esp, io_regs, io_env, io_mxcsr, io_out, allocator_object, io_crashed
io_crashed: .long 0
saved_esp: .long 0
io_entry: .long 0
io_esp: .long 0
io_regs: .long 0,0,0,0
io_env: .long 0,0,0,0,0,0,0
io_mxcsr: .long 0
io_out: .long 0,0,0,0,0,0,0,0
allocator_object: .long vtable, 0
vtable: .long 0,0,0,0, alloc, release, 0, 0
