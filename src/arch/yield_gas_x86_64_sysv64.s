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
 *
 * Windows:
 *   Integer args : RCX, RDX, R8, R9
 *   Volatile     : RAX, RCX, RDX, R8-R11, XMM0-XMM5
 */

.section .data
  .align 16
  main_stack: .long 0

.section .text
  .extern casync_task_switch
  .global casync_yield
  .global casync_restore

.section .data
  .extern casync_current_loop

.macro SAVE_CONTEXT
  pushq  %rax
  pushq  %rcx
  pushq  %rdx
  pushq  %rbx
  pushq  %rbp
  pushq  %rsi
  pushq  %rdi
  pushq  %r8
  pushq  %r9
  pushq  %r10
  pushq  %r11
  pushq  %r12
  pushq  %r13
  pushq  %r14
  pushq  %r15
  pushfq

  # *casync_current_loop->active->stack = rsp
  #movq   casync_current_loop@GOTPCREL(%rip), %rax  # load struct from GOT
  #movq   (%rax), %rax        # Get first field in struct ("active")
  movq   %fs:casync_current_loop@TPOFF, %rax
  movq   (%rax), %rax        # Get first field in struct ("stack")
  movq   %rsp, (%rax)        # Assign current stack pointer to field in struct
.endm

.macro RESTORE_CONTEXT
  # rsp = *casync_current_loop->active->stack
  #movq   casync_current_loop@GOTPCREL(%rip), %rax  # Load struct from GOT
  #movq   (%rax), %rax        # Get first field in struct ("active")
  movq   %fs:casync_current_loop@TPOFF, %rax
  movq   (%rax), %rax        # Get first field in struct ("stack")
  movq   (%rax), %rsp        # Assign stored stack pointer to rsp

  popfq
  popq  %r15
  popq  %r14
  popq  %r13
  popq  %r12
  popq  %r11
  popq  %r10
  popq  %r9
  popq  %r8
  popq  %rdi
  popq  %rsi
  popq  %rbp
  popq  %rbx
  popq  %rdx
  popq  %rcx
  popq  %rax
.endm

.section .text
casync_yield:
  SAVE_CONTEXT               # Save context of the current task
  leaq  -8(%rsp), %rsp       # Align
  call  casync_task_switch   # Call scheduler
casync_restore:
  RESTORE_CONTEXT            # Restore context of new task
  ret                        # "Return" to the task function

