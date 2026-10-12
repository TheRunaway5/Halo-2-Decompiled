"""Test-only original CPU input/capture bridge. No callee or phase results.

Derived from the existing owned generic i386 test bridge; its unused phase
re-entry/GP/stack substitution path is removed. Original image bytes are a
separate unchanged section. Production never uses this Linux harness.
"""

ASM = r'''
.code32
.section .bridge,"awx"
.globl _start
_start:
 mov $harness_stack_end,%esp
loop:
 mov $input,%ecx
 mov $288,%edx
 call read_exact
 mov $phase,%ecx
 mov $92,%edx
 call read_exact
 mov input+284,%ebp
 mov $banks,%edi
bank_read:
 test %ebp,%ebp
 jz setup
 mov %edi,%ecx
 mov $8,%edx
 call read_exact
 mov (%edi),%ecx
 mov 4(%edi),%edx
 call read_exact
 add $8,%edi
 dec %ebp
 jmp bank_read
setup:
 mov $0x100f000,done_entry
 frstor input+48
 ldmxcsr input+44
 movdqu input+156,%xmm0
 movdqu input+172,%xmm1
 movdqu input+188,%xmm2
 movdqu input+204,%xmm3
 movdqu input+220,%xmm4
 movdqu input+236,%xmm5
 movdqu input+252,%xmm6
 movdqu input+268,%xmm7
 push input+40
 popfl
 mov input+8,%eax
 mov input+12,%ecx
 mov input+16,%edx
 mov input+20,%ebx
 mov input+28,%ebp
 mov input+32,%esi
 mov input+36,%edi
 mov input+24,%esp
 int3
 jmp *input+4
.globl capture
capture:
 mov %eax,output
 mov %ecx,output+4
 mov %edx,output+8
 mov %ebx,output+12
 mov %esp,output+16
 mov %ebp,output+20
 mov %esi,output+24
 mov %edi,output+28
 mov $harness_stack_end,%esp
 pushfl
 pop output+32
 stmxcsr output+36
 fnsave output+40
 movdqu %xmm0,output+148
 movdqu %xmm1,output+164
 movdqu %xmm2,output+180
 movdqu %xmm3,output+196
 movdqu %xmm4,output+212
 movdqu %xmm5,output+228
 movdqu %xmm6,output+244
 movdqu %xmm7,output+260
 mov $harness_stack_end,%esp
emit:
 mov $output,%ecx
 mov $280,%edx
 call write_exact
 mov input+284,%ebp
 mov $banks,%edi
bank_write:
 test %ebp,%ebp
 jz loop
 mov (%edi),%ecx
 mov 4(%edi),%edx
 call write_exact
 add $8,%edi
 dec %ebp
 jmp bank_write
read_exact:
 test %edx,%edx
 jz io_return
 mov $3,%eax
 xor %ebx,%ebx
 int $0x80
 test %eax,%eax
 jle exit
 add %eax,%ecx
 sub %eax,%edx
 jmp read_exact
write_exact:
 test %edx,%edx
 jz io_return
 mov $4,%eax
 mov $1,%ebx
 int $0x80
 test %eax,%eax
 jle failure
 add %eax,%ecx
 sub %eax,%edx
 jmp write_exact
io_return:
 ret
failure:
 mov $1,%ebx
 jmp terminate
exit:
 xor %ebx,%ebx
terminate:
 mov $1,%eax
 int $0x80
.balign 16
input: .space 288
phase: .space 92
output: .space 276
done_entry: .long 0
banks: .space 65536
harness_stack: .space 65536
harness_stack_end:
'''
