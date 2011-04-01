/*
 * Low-level context manipulation routines for AMD64.
 * They are optimized to save
 * only callee-save registers and omit FP context altogether.
 *
 * Stack is used to store param and callee-saved regs
 * http://charm.cs.illinois.edu/papers/migThreads.www/node24.html
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
                # context IS just the displaced stack pointer;
                # displaced to account for popping off in swap()
		leaq		-56(%rsi), %rax
                ##
                movq            %rax, 0(%rdi)
                # write entry function to new stack (as return address)
		movq		%rdx, 0(%rsi)
                # write func arg to new stack
		movq		%rcx, -8(%rsi)
		ret

		.align 16
ctx_swap_internal:
		/*
		rdi = old context address
		rsi = new context address
		*/
                # caller saved registers
		pushq		%rdi
		pushq		%rbp
		pushq		%rbx
		pushq		%r12
		pushq		%r13
		pushq		%r14
		pushq		%r15
                # stack pointer
		movq		%rsp, (%rdi)
                # load stack pointer (switch to new stack)
		movq		(%rsi), %rsp
                # restore callee saved registers
		popq		%r15
		popq		%r14
		popq		%r13
		popq		%r12
		popq		%rbx
		popq		%rbp
		popq		%rdi
		ret
