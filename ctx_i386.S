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
  # 4(%esp) = context address
  # 8(%esp) = stack pointer
  # 12(%esp) = entry point address
  # 16(%esp) = argument to the entry point
  #
  # store stack address in eax
  mov     8(%esp), %eax
  # context IS just the displaced stack pointer;
  # displaced to account for popping off in swap()
  lea     -24(%eax), %ecx
  mov     4(%esp), %edx
  mov     %ecx, (%edx)
  # write entry function to new stack (as return address)
  mov     12(%esp), %ecx
  mov     %ecx, -8(%eax)
  # write func arg to new stack
  mov     16(%esp), %ecx
  mov     %ecx,  0(%eax)
  # write fake return address
  #movl    $0xdead, -4(%eax)
  ret

  .align 16
ctx_swap_internal:
  # 4(%esp) = old context address
  # 8(%esp) = new context address
  mov     4(%esp), %eax
  mov     8(%esp), %ecx
  # caller saved registers
  push    %ebp
  push    %ebx
  push    %esi
  push    %edi
  # stack pointer
  mov     %esp, (%eax)
  # load stack pointer (switch to new stack)
  mov     (%ecx), %esp
  # restore callee saved registers
  pop     %edi
  pop     %esi
  pop     %ebx
  pop     %ebp
  ret

