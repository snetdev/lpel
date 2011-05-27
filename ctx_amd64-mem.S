/*
 * Low-level context manipulation routines for AMD64.
 * They are optimized to save
 * only callee-save registers and omit FP context altogether.
 *
 * This file is taken and modified from Nornir, (c) Zeljko Vrba
 *
 * 2011-04-01 Daniel Prokesch
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
		movq		%rsi, 0(%rdi)
		movq		%rdx, 8(%rdi)
		movq		%rcx, 16(%rdi)
		ret

		.align 16
ctx_swap_internal:
		/*
		rdi = old context address
		rsi = new context address
		*/
                # caller saved registers
		movq		%r15, 24(%rdi)
		movq		%r14, 32(%rdi)
		movq		%r13, 40(%rdi)
		movq		%r12, 48(%rdi)
		movq		%rbp, 56(%rdi)
		movq		%rbx, 64(%rdi)
                # stack pointer
		movq		%rsp, 0(%rdi)
                # return address (on TOS)
		movq		(%rsp),  %rax
		movq		%rax, 8(%rdi)
                # load stack pointer (switch to new stack)
		movq		0(%rsi), %rsp
                # restore return address
		movq		8(%rsi), %rax
		movq		%rax, (%rsp)
                # restore callee saved registers
		movq		24(%rsi), %r15
		movq		32(%rsi), %r14
		movq		40(%rsi), %r13
		movq		48(%rsi), %r12
		movq		56(%rsi), %rbp
		movq		64(%rsi), %rbx
                # restore first argument (only useful upon first call?)
		movq		16(%rsi), %rdi
		ret
