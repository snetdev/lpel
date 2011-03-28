/*
Low-level context manipulation routines for AMD64.  They are optimized to save
only callee-save registers and omit FP context altogether.
*/

		.globl ctx_init_internal
		.globl ctx_swap_internal

		.type ctx_init_internal, @function
		.type ctx_swap_internal, @function

		.section .text

		.align 16
ctx_init_internal:
		/*
		rdi = context address
		rsi = stack pointer
		rdx = entry point address
		rcx = argument to the entry point
		*/
		movq		%rsi, 120(%rdi)
		movq		%rdx, 136(%rdi)
		movq		%rcx, 64(%rdi)
		ret

		.align 16
ctx_swap_internal:
		/*
		rdi = old context address
		rsi = new context address
		*/
		movq		%r15, 0(%rdi)
		movq		%r14, 8(%rdi)
		movq		%r13, 16(%rdi)
		movq		%r12, 24(%rdi)
		movq		%rbp, 80(%rdi)
		movq		%rbx, 88(%rdi)
		movq		%rsp, 120(%rdi)
		movq		(%rsp),  %rax		/* rip */
		movq		%rax, 136(%rdi)
		movq		120(%rsi), %rsp		/* switch to new stack */
		movq		136(%rsi), %rax		/* rip */
		movq		%rax, (%rsp)
		movq		0(%rsi), %r15
		movq		8(%rsi), %r14
		movq		16(%rsi), %r13
		movq		24(%rsi), %r12
		movq		80(%rsi), %rbp
		movq		88(%rsi), %rbx
		movq		64(%rsi), %rdi		/* restore 1st argument */
		ret
