# Some notes:
#   - Floating point registers are not saved/restored
#   - SIMD registers are not saved/restored
#   - When calling C functions, the stack pointer must always be aligned to 16
#     bytes prior to the call. Any C code calling assembly routines will have
#     aligned the stack pointer to 16 bytes as well. This makes it straight
#     forward to adjust the stack pointer. On function entry, the stack pointer
#     will be -4 bytes due to the return address.

.section .note.GNU-stack

/*
 * Linux (System V AMD64 ABI):
 *   Integer args : RDI, RSI, RDX, RCX, R8, R9
 *   Volatile     :
 */

.section .data
  .extern casync_current_loop

.section .text
  .extern casync_end
  .global casync_yield
  .global casync_restore
  .global casync_end_redirect

.macro LOAD_TLS var_name
  movq    %fs:\var_name@TPOFF, %rax
.endm

.macro LOAD_GLOBAL var_name
  movq    \var_name@GOTPCREL(%rip)
  movq    (\reg), \reg
.endm

casync_end_redirect:
  movl    %eax, %edi
  jmp     casync_end

casync_yield:
  pushfq
  pushq   %rax

  LOAD_TLS casync_current_loop
  test    %rax, %rax
  je      .no_loop

  pushq   %rcx
  pushq   %rdx
  pushq   %rbx
  pushq   %rbp
  pushq   %rsi
  pushq   %rdi
  pushq   %r8
  pushq   %r9
  pushq   %r10
  pushq   %r11
  pushq   %r12
  pushq   %r13
  pushq   %r14
  pushq   %r15

  movq    (%rax), %rdx        # rdx = casync_current_loop->active
  movq    8(%rdx), %rcx       # rcx = casync_current_loop->active->next
  movq    %rsp, (%rdx)        # casync_current_loop->active->stack = rsp
  movq    %rcx, (%rax)        # casync->current_loop->active = rcx

casync_restore:
  LOAD_TLS casync_current_loop
  movq    (%rax), %rdx        # rdx = casync_current_loop->active
  movq    (%rdx), %rsp        # rsp = casync_current_loop->active->stack
  popq    %r15
  popq    %r14
  popq    %r13
  popq    %r12
  popq    %r11
  popq    %r10
  popq    %r9
  popq    %r8
  popq    %rdi
  popq    %rsi
  popq    %rbp
  popq    %rbx
  popq    %rdx
  popq    %rcx
.no_loop:
  popq    %rax
  popfq
  ret

