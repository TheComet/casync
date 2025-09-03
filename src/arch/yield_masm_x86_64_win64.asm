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
  EXTERN casync_end: PROC
  PUBLIC casync_yield
  PUBLIC casync_restore
  PUBLIC casync_end_redirect

LOAD_TLS MACRO var_name
  mov     ecx, DWORD PTR _tls_index
  mov     rax, gs:[58h]
  mov     ebx, SECTIONREL var_name
  movsxd  rbx, ebx
  mov     rdx, [rax+rcx*8]
  mov     rax, [rdx+rbx]
ENDM

casync_end_redirect PROC
  mov     ecx, eax
  jmp     casync_end
casync_end_redirect ENDP

casync_yield PROC
  pushfq
  push    rax
  push    rcx
  push    rdx
  push    rbx

  LOAD_TLS casync_current_loop
  test    rax, rax
  je      .no_loop

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

  mov     rdx, [rax]          ; rdx = casync_current_loop->active
  mov     rcx, [rdx+8]        ; rcx = casync_current_loop->active->next
  mov     [rdx], rsp          ; casync_current_loop->active->stack = rsp
  mov     [rax], rcx          ; casync->current_loop->active = rcx

casync_restore PROC
  LOAD_TLS rax, casync_current_loop
  mov     rdx, [rax]          ; rdx = casync_current_loop->active
  mov     rsp, [rdx]          ; rsp = casync_current_loop->active->stack
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
.no_loop:
  pop     rbx
  pop     rdx
  pop     rcx
  pop     rax
  popfq
  ret
casync_restore ENDP
casync_yield ENDP

END
