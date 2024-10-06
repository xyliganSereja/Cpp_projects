stdout:             equ             1
stderr:             equ             2

sys_exit:           equ             60
sys_write:          equ             1

buf_size:           equ             8192

                    section         .text
                    global          _start
_start:
                    xor             ebx, ebx        ;counter
                    xor             r8, r8          ;is first space
read_iter:
                    xor             eax, eax
                    xor             edi, edi
                    mov             rsi, read_buffer
                    mov             rdx, buf_size
                    syscall

                    test            rax, rax
                    jl              io_err
                    je              finish_reading


                    xor             ecx, ecx        ;iteration counter
next_char:

                    movzx           ebp, byte [read_buffer + rcx]
                    cmp             ebp, 9
                    jl              skip_inc2

                    cmp             ebp, 13
                    jng             do_inc

                    cmp             ebp, 32
                    jne             skip_inc2
do_inc:
                    add             rbx, r8
                    xor             r8, r8
                    jmp             skip_inc
skip_inc2:
                    mov             r8, 1
skip_inc:
                    inc             rcx

                    cmp             rcx, rax
                    je              read_iter

                    jmp             next_char


finish_reading:
                    add             rbx, r8
                    mov             rcx, str_buffer + str_buffer_size - 1        ;iteration counter
                    mov             byte [rcx], 10

                    mov             rax, rbx
                    mov             rbx, 10
to_str_iter:
                    dec             rcx
                    xor             edx, edx
                    div             rbx
                    add             dl, '0'
                    mov             [rcx], dl

                    test            rax, rax
                    jne             to_str_iter

                    mov             rbx, str_buffer_size + str_buffer
                    sub             rbx, rcx

                    mov             eax, sys_write
                    mov             edi, stdout
                    mov             rsi, rcx
                    mov             rdx, rbx
                    syscall

                    mov             eax, sys_exit
                    xor             edi, edi
                    syscall


io_err:
                    mov             eax, sys_write
                    mov             rdi, stderr
                    mov             rsi, error_msg
                    mov             rdx, error_msg_size
                    syscall

                    mov             eax, sys_exit
                    mov             edi, 1
                    syscall

                    
                    section         .rodata
error_msg:          db              "Error reading from stdin", 0x0a
error_msg_size:     equ             $ - error_msg
str_buffer_size:    equ             20

                    section         .bss
read_buffer:        resb             buf_size
str_buffer:         resb             str_buffer_size
