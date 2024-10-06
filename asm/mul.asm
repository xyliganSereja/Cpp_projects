                section         .text

                global          _start

mult_len_qwords equ             128
mult_len_bytes  equ             mult_len_qwords * 8
res_len_qwords  equ             mult_len_qwords * 2
res_len_bytes   equ             res_len_qwords * 2
data_size       equ             3 * mult_len_bytes
mult1_addr      equ             2 * mult_len_bytes
mult2_addr      equ             mult_len_bytes



_start:

                sub             rsp, data_size
                lea             rdi, [rsp + mult1_addr]
                mov             rcx, mult_len_qwords
                call            read_long
                lea             rdi, [rsp + mult2_addr]
                call            read_long
                mov             rsi, rdi
                lea             rdi, [rsp + mult1_addr]
                call            multiply_long_long

                mov             rcx, res_len_qwords
                mov             rdi, rsp
                call            write_long

                mov             al, 0x0a
                call            write_char

                jmp             exit

; multiplies two long numbers
;    rdi -- address of multiplier #1 (long number)
;    rsi -- address of multiplier #2 (long number)
;    rcx -- length of long numbers in qwords
;
;    result is writen in range [rsi - 8 * rcx : rsi + 8 * (rcx - 1) ]
multiply_long_long:
                push            rcx
                push            rsi
                push            rdi


                ; result of multiplication is two-part long number (2 * rcx qwords)
                ; second part (higher qwords) will be written over second multiplier
                ; first part (lower qwords) will be written just before it (at [rsi - 8 * rcx : rsi - 8])


                ; rbx -- destination address
                lea             rbx, [8 * rcx]
                neg             rbx
                lea             rbx, [rsi + rbx]

                mov             r8, rcx

                push            rdi
                mov             rdi, rbx
                call            set_zero
                pop             rdi

                ; algorithm is similar to column method of multiplication:
                ; first operand multiplies by qword of second operand (by address rsi)
                ; r8 -- counter (r8 \in [rcx...1])
.loop1:
                push            rbx
                push            rdi

                xor             r13, r13            ; carry register
.loop2:
                mov             rax, [rdi]
                mul             qword [rsi]
                add             [rbx], r13
                mov             r13, rdx
                adc             r13, 0
                add             [rbx], rax
                adc             r13, 0
                add             rbx, 8
                add             rdi, 8
                cmp             rbx, rsi
                jne             .loop2
; loop2 end

                pop             rdi
                pop             rbx
                mov             qword [rsi], r13
                add             rsi, 8
                add             rbx, 8
                dec             r8
                test            r8, r8
                jnz             .loop1
; loop1 end

                pop             rdi
                pop             rsi
                pop             rcx
                ret


; adds 64-bit number to long number
;    rdi -- address of summand #1 (long number)
;    rax -- summand #2 (64-bit unsigned)
;    rcx -- length of long number in qwords
; result:
;    sum is written to rdi
add_long_short:
                push            rdi
                push            rcx
                push            rdx

                xor             rdx,rdx
.loop:
                add             [rdi], rax
                adc             rdx, 0
                mov             rax, rdx
                xor             rdx, rdx
                add             rdi, 8
                dec             rcx
                jnz             .loop

                pop             rdx
                pop             rcx
                pop             rdi
                ret

; multiplies long number by a short
;    rdi -- address of multiplier #1 (long number)
;    rbx -- multiplier #2 (64-bit unsigned)
;    rcx -- length of long number in qwords
; result:
;    product is written to rdi
mul_long_short:
                push            rax
                push            rdi
                push            rcx

                xor             rsi, rsi
.loop:
                mov             rax, [rdi]
                mul             rbx
                add             rax, rsi
                adc             rdx, 0
                mov             [rdi], rax
                add             rdi, 8
                mov             rsi, rdx
                dec             rcx
                jnz             .loop

                pop             rcx
                pop             rdi
                pop             rax
                ret

; divides long number by a short
;    rdi -- address of dividend (long number)
;    rbx -- divisor (64-bit unsigned)
;    rcx -- length of long number in qwords
; result:
;    quotient is written to rdi
;    rdx -- remainder
div_long_short:
                push            rdi
                push            rax
                push            rcx

                lea             rdi, [rdi + 8 * rcx - 8]
                xor             rdx, rdx

.loop:
                mov             rax, [rdi]
                div             rbx
                mov             [rdi], rax
                sub             rdi, 8
                dec             rcx
                jnz             .loop

                pop             rcx
                pop             rax
                pop             rdi
                ret

; assigns a zero to long number
;    rdi -- argument (long number)
;    rcx -- length of long number in qwords
set_zero:
                push            rax
                push            rdi
                push            rcx

                xor             rax, rax
                rep stosq

                pop             rcx
                pop             rdi
                pop             rax
                ret

; checks if a long number is a zero
;    rdi -- argument (long number)
;    rcx -- length of long number in qwords
; result:
;    ZF=1 if zero
is_zero:
                push            rax
                push            rdi
                push            rcx

                xor             rax, rax
                rep scasq

                pop             rcx
                pop             rdi
                pop             rax
                ret

; read long number from stdin
;    rdi -- location for output (long number)
;    rcx -- length of long number in qwords
read_long:
                push            rcx
                push            rdi

                call            set_zero
.loop:
                call            read_char
                or              rax, rax
                js              exit
                cmp             rax, 0x0a
                je              .done
                cmp             rax, '0'
                jb              .invalid_char
                cmp             rax, '9'
                ja              .invalid_char

                sub             rax, '0'
                mov             rbx, 10
                call            mul_long_short
                call            add_long_short
                jmp             .loop

.done:
                pop             rdi
                pop             rcx
                ret

.invalid_char:
                mov             rsi, invalid_char_msg
                mov             rdx, invalid_char_msg_size
                call            print_string
                call            write_char
                mov             al, 0x0a
                call            write_char

.skip_loop:
                call            read_char
                or              rax, rax
                js              exit
                cmp             rax, 0x0a
                je              exit
                jmp             .skip_loop

; write long number to stdout
;    rdi -- argument (long number)
;    rcx -- length of long number in qwords
write_long:
                push            rax
                push            rcx

                mov             rax, 20
                mul             rcx
                mov             rbp, rsp
                sub             rsp, rax

                mov             rsi, rbp

.loop:
                mov             rbx, 10
                call            div_long_short
                add             rdx, '0'
                dec             rsi
                mov             [rsi], dl
                call            is_zero
                jnz             .loop

                mov             rdx, rbp
                sub             rdx, rsi
                call            print_string

                mov             rsp, rbp
                pop             rcx
                pop             rax
                ret

; read one char from stdin
; result:
;    rax == -1 if error occurs
;    rax \in [0; 255] if OK
read_char:
                push            rcx
                push            rdi

                sub             rsp, 1
                xor             rax, rax
                xor             rdi, rdi
                mov             rsi, rsp
                mov             rdx, 1
                syscall

                cmp             rax, 1
                jne             .error
                xor             rax, rax
                mov             al, [rsp]
                add             rsp, 1

                pop             rdi
                pop             rcx
                ret
.error:
                mov             rax, -1
                add             rsp, 1
                pop             rdi
                pop             rcx
                ret

; write one char to stdout, errors are ignored
;    al -- char
write_char:
                sub             rsp, 1
                mov             [rsp], al

                mov             rax, 1
                mov             rdi, 1
                mov             rsi, rsp
                mov             rdx, 1
                syscall
                add             rsp, 1
                ret

exit:
                mov             rax, 60
                xor             rdi, rdi
                syscall

; print string to stdout
;    rsi -- string
;    rdx -- size
print_string:
                push            rax

                mov             rax, 1
                mov             rdi, 1
                syscall

                pop             rax
                ret


                section         .rodata
invalid_char_msg:
                db              "Invalid character: "
invalid_char_msg_size: equ             $ - invalid_char_msg
