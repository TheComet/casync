# Some notes:
#   - Floating point registers are not saved/restored
#   - SIMD registers are not saved/restored
#   - When calling C functions, the stack pointer must always be aligned to 16
#     bytes prior to the call. Any C code calling assembly routines will have
#     aligned the stack pointer to 16 bytes as well. This makes it straight
#     forward to adjust the stack pointer. On function entry, the stack pointer
#     will be -4 bytes due to the return address.

.section .note.GNU-stack

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
  pushal                    # eax, ecx, edx, ebx, esp, ebp, esi, edi
  pushfl                    # eflags register

  # *casync_current_loop->active->stack = esp
  #movl  casync_current_loop@GOT(%ebx), %eax  # load struct from GOT
  #movl  (%eax), %eax        # Get first field in struct ("active")
  movl  %gs:casync_current_loop@NTPOFF, %eax
  movl  (%eax), %eax        # Get first field in struct ("stack")
  movl  %esp, (%eax)        # Assign current stack pointer to field in struct
.endm

.macro RESTORE_CONTEXT
  # esp = *casync_current_loop->active->stack
  #movl  casync_current_loop@GOT(%ebx), %eax  # Load struct from GOT
  #movl  (%eax), %eax        # Get first field in struct ("active")
  movl  %gs:casync_current_loop@NTPOFF, %eax
  movl  (%eax), %eax        # Get first field in struct ("stack")
  movl  (%eax), %esp        # Assign stored stack pointer to esp

  popfl                     # eflags register
  popal                     # eax, ecx, edx, ebx, esp, ebp, esi, edi
.endm

.section .text
casync_yield:
  SAVE_CONTEXT              # Save context of the current task
  leal  -4(%esp), %esp      # return address(4) + context(8) + 4 = 16
  call  casync_task_switch  # Call scheduler
casync_restore:
  RESTORE_CONTEXT           # Restore context of new task
  ret                       # "Return" to the task function
