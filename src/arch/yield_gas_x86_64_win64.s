# Some notes:
#   - Floating point registers are not saved/restored
#   - SIMD registers are not saved/restored
#   - When calling C functions, the stack pointer must always be aligned to 16
#     bytes prior to the call. Any C code calling assembly routines will have
#     aligned the stack pointer to 16 bytes as well. This makes it straight
#     forward to adjust the stack pointer. On function entry, the stack pointer
#     will be -4 bytes due to the return address.

# Windows x64 ABI:
#   Integer args : RCX, RDX, R8, R9
#   Volatile     : RAX, RCX, RDX, R8-R11, XMM0-XMM5

.section .data
  .extern casync_current_loop

.section .text
  .extern casync_end
  .global casync_yield
  .global casync_restore
  .global casync_end_redirect

# Loads a TLS variable into %rax
.macro LOAD_TLS var_name
  .extern __emutls_v.\var_name:
  subq    $40, %rsp
  leaq    __emutls_v.\var_name(%rip), %rcx
  call    __emutls_get_address
  movq    (%rax), %rax
  addq    $40, %rsp
  ret
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
  LOAD_TLS casync_current_loop
  movq    (%rax), %rax        # Get first field in struct ("stack")
  movq    %rsp, (%rax)        # Assign current stack pointer to field in struct
.endm

.macro RESTORE_CONTEXT
  LOAD_TLS casync_current_loop
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
  movl    %eax, %ecx
  jmp     casync_end

casync_yield:
  SAVE_CONTEXT
  leaq    -8(%rsp), %rsp     # Align
  call    casync_task_switch # Call scheduler
casync_restore:
  RESTORE_CONTEXT            # Restore context of new task
  ret                        # "Return" to the task function
