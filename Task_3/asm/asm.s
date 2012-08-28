.data
scanf_format:
	.string "%lu"
	
printf_format:
	.string "%lu fibonacci number is %lu.\n"
	
hello:
	.string "Hello. Enter number, please: "
	
error:
	.string "I can't calculate this fibonacci number... ):\n"

error_input:
	.string "I think it is incorrect input... ):\n"

tmp:
	.quad 0
	
count:
	.quad 1
	
.text

.globl main
main:
	movq $hello, %rdi
	movq $0, %rax
	call printf
	movq $count, %rsi
	movq $scanf_format, %rdi
	movq $0, %rax
	call scanf
	
	cmpq $0, %rax
	jne all_ok
	movq $error_input, %rdi
	movq $0, %rax
	call printf
	pushq $1
	call exit
	
all_ok:
	movq count, %rcx
	movq $1, %r9
	movq $0, %r10
	
fib:
	movq %r10, tmp
	addq %r9, %r10
	
	jno	 no_carry
	movq $error, %rdi
	movq $0, %rax
	call printf
	pushq $1
	call exit
	
no_carry:
	movq tmp, %r9
	pushq %r10
	
	loop fib
	
	addq $1, count
fib_out:
	subq $1, count
	movq $printf_format, %rdi
	movq count, %rsi
	popq %rdx
	movq $0, %rax
	call printf
	movq count, %rcx
	loop fib_out

	pushq $0
	call exit
