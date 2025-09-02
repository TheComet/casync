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
  .extern casync_task_switch
  .extern casync_end
  .global casync_yield
  .global casync_restore
  .global casync_end_redirect

.macro LOAD_TLS var_name reg
  movq    %fs:\var_name@TPOFF, \reg
.endm

.macro LOAD_GLOBAL var_name reg
  movq    \var_name@GOTPCREL(%rip), \reg
  movq    (\reg), \reg
.endm

.macro SAVE_CONTEXT
  pushq   %rax
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
  pushfq

  # *casync_current_loop->active->stack = rsp
  LOAD_TLS casync_current_loop, %rax
  movq    (%rax), %rax        # Get first field in struct ("stack")
  movq    %rsp, (%rax)        # Assign current stack pointer to field in struct
.endm

.macro RESTORE_CONTEXT
  LOAD_TLS casync_current_loop, %rax
  movq    (%rax), %rax        # Get first field in struct ("stack")
  movq    (%rax), %rsp        # Assign stored stack pointer to rsp

  popfq
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
  popq    %rax
.endm

casync_end_redirect:
  movl    %eax, %edi
  jmp     casync_end

casync_yield:
  pushq   %rax
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
  pushfq

  # *casync_current_loop->active->stack = rsp
  LOAD_TLS casync_current_loop, %rax
  movq    (%rax), %rax        # Get first field in struct ("stack")
  movq    %rsp, (%rax)        # Assign current stack pointer to field in struct
  leaq    -8(%rsp), %rsp     # Align
  call    casync_task_switch # Call scheduler
casync_restore:
  RESTORE_CONTEXT            # Restore context of new task
  ret                        # "Return" to the task function

