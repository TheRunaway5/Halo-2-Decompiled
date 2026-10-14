.text
.globl guest_run
.type guest_run,@function
guest_run:
 pushal
 pushfl
 movl %esp,guest_host_stack
 movw %fs,host_selector
 movw guest_selector,%ax
 movw %ax,%fs
 fxsave guest_host_fp
 fninit
 fldcw guest_cw
 ldmxcsr guest_mxcsr
 movdqu guest_input+32,%xmm0
 movdqu guest_input+48,%xmm1
 movdqu guest_input+64,%xmm2
 movdqu guest_input+80,%xmm3
 movdqu guest_input+96,%xmm4
 movdqu guest_input+112,%xmm5
 movdqu guest_input+128,%xmm6
 movdqu guest_input+144,%xmm7
 pushl guest_input+160
 popfl
 movl guest_input+0,%eax
 movl guest_input+4,%ecx
 movl guest_input+8,%edx
 movl guest_input+12,%ebx
 movl guest_input+20,%ebp
 movl guest_input+24,%esi
 movl guest_input+28,%edi
 movl guest_input+16,%esp
 jmp *guest_entry
.size guest_run,.-guest_run
.globl guest_return
.type guest_return,@function
guest_return:
 movl %eax,guest_exit+0
 movl %ecx,guest_exit+4
 movl %edx,guest_exit+8
 movl %ebx,guest_exit+12
 movl %esp,guest_exit+16
 movl %ebp,guest_exit+20
 movl %esi,guest_exit+24
 movl %edi,guest_exit+28
 movdqu %xmm0,guest_exit+32
 movdqu %xmm1,guest_exit+48
 movdqu %xmm2,guest_exit+64
 movdqu %xmm3,guest_exit+80
 movdqu %xmm4,guest_exit+96
 movdqu %xmm5,guest_exit+112
 movdqu %xmm6,guest_exit+128
 movdqu %xmm7,guest_exit+144
 pushfl
 popl guest_exit+160
 movl $0x60200000,guest_exit+164
 fnstsw guest_exit+172
 fnstcw guest_exit+174
 stmxcsr guest_exit+176
 fnstenv guest_environment
 fxrstor guest_host_fp
 movw host_selector,%ax
 movw %ax,%fs
 movl guest_host_stack,%esp
 popfl
 popal
 ret
.size guest_return,.-guest_return
.section .note.GNU-stack,"",@progbits
