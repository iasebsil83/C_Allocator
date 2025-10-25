.intel_syntax noprefix


/* general syscall-as-fct tool */
.global fct_to_syscall
fct_to_syscall:
	mov r10, rcx
	syscall
	ret


/* mmap - munmap */
.global syscall_mmap
syscall_mmap:
	mov	rax, 0x00000009
	jmp fct_to_syscall
	ret

.global syscall_munmap
syscall_munmap:
	mov	rax, 0x0000000b
	jmp fct_to_syscall
	ret



/* output */
.global syscall_write
syscall_write:
	mov	rax, 0x00000001
	jmp fct_to_syscall
	ret



/* exit */
.global syscall_exit
syscall_exit:
	mov	rax, 0x0000003c
	jmp fct_to_syscall
	ret
