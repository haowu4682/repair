.text
.align 16, 0x90
.globl do_syscall
.type do_syscall,@function
do_syscall:
	// calling convention args: rdi, rsi, rdx, rcx, r8, r9
	movq	8(%rsp), %rax	// syscall#
	syscall
	retq
	// XXX errno missing

