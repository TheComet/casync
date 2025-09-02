; Some notes:
;   - Floating point registers are not saved/restored
;   - SIMD registers are not saved/restored
;   - When calling C functions, the stack pointer must always be aligned to 16
;     bytes prior to the call. Any C code calling assembly routines will have
;     aligned the stack pointer to 16 bytes as well. This makes it straight
;     forward to adjust the stack pointer. On function entry, the stack pointer
;     will be -4 bytes due to the return address.

; Windows x64 ABI:
;   Integer args : RCX, RDX, R8, R9
;   Volatile     : RAX, RCX, RDX, R8-R11, XMM0-XMM5

.data
  EXTERN casync_current_loop: QWORD
  EXTERN _tls_index: DWORD

.code
  EXTERN casync_task_switch: PROC
  EXTERN casync_end: PROC
  PUBLIC casync_yield
  PUBLIC casync_restore
  PUBLIC casync_end_redirect

LOAD_TLS MACRO reg, var_name
  mov     ecx, DWORD PTR _tls_index
  mov     rax, gs:[58h]
  mov     ebx, SECTIONREL var_name
  movsxd  rbx, ebx
  mov     rdx, [rax+rcx*8]
  mov     reg, [rdx+rbx]
ENDM

SAVE_CONTEXT MACRO
  push    rax
  push    rcx
  push    rdx
  push    rbx
  push    rbp
  push    rsi
  push    rdi
  push    r8
  push    r9
  push    r10
  push    r11
  push    r12
  push    r13
  push    r14
  push    r15
  pushfq

  ; *casync_current_loop->active->stack = rsp
  LOAD_TLS rax, casync_current_loop
  mov     rax, [rax]          ; Get first field in struct ("stack")
  mov     [rax], rsp          ; Assign current stack pointer to field in struct
ENDM

RESTORE_CONTEXT MACRO
  ; rsp = *casync_current_loop->active->stack
  LOAD_TLS rax, casync_current_loop
  mov     rax, [rax]          ; Get first field in struct ("stack")
  mov     rsp, [rax]          ; Assign stored stack pointer to rsp

  popfq
  pop     r15
  pop     r14
  pop     r13
  pop     r12
  pop     r11
  pop     r10
  pop     r9
  pop     r8
  pop     rdi
  pop     rsi
  pop     rbp
  pop     rbx
  pop     rdx
  pop     rcx
  pop     rax
ENDM

casync_end_redirect PROC
  mov     ecx, eax
  jmp     casync_end
casync_end_redirect ENDP

casync_yield PROC
  SAVE_CONTEXT                ; Save context of the current task
  lea     rsp, [rsp - 8 - 32] ; Align + reserve 32 bytes of shadow space
  call    casync_task_switch  ; Call scheduler
  RESTORE_CONTEXT             ; Restore context of new task
  ret                         ; "Return" to the task function
casync_yield ENDP

casync_restore PROC
  RESTORE_CONTEXT             ; Restore context of new task
  ret                         ; "Return" to the task function
casync_restore ENDP

END
