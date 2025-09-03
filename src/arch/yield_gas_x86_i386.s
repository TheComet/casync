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
  .extern casync_current_loop

.section .text
  .extern casync_end
  .global casync_end_redirect
  .global casync_yield
  .global casync_restore

.macro LOAD_TLS var_name
  movl    %gs:\var_name@NTPOFF, %eax
.endm

.macro LOAD_TLS_WIN32 var_name
  .extern __emutls_v.\var_name:
  subl    $40, %esp
  leal    __emutls_v.\var_name(%ebx), %ecx
  call    __emutls_get_address
  addl    $40, %esp
  ret
.endm

.macro LOAD_GLOBAL var_name reg
  movl    \var_name@GOT(%ebx), \reg
  movl    (\reg), \reg
.endm

casync_end_redirect:
  pushl   %eax
  pushl   $0
  jmp     casync_end

casync_yield:
  pushfl                      # eflags register
  pushal                      # eax, ecx, edx, ebx, esp, ebp, esi, edi

  LOAD_TLS casync_current_loop
  test    %eax, %eax
  je      .no_loop

  movl    (%eax), %edx        # edx = casync_current_loop->active
  movl    4(%edx), %ecx       # rcx = casync_current_loop->active->next
  movl    %esp, (%edx)        # casync_current_loop->active->stack = esp
  movl    %ecx, (%eax)        # casync_current_loop->active = ecx
casync_restore:
  LOAD_TLS casync_current_loop
  movl    (%eax), %edx        # Get first field in struct ("stack")
  movl    (%edx), %esp        # Assign stored stack pointer to esp
.no_loop:
  popal                       # eax, ecx, edx, ebx, esp, ebp, esi, edi
  popfl                       # eflags register
  ret                         # "Return" to the task function
