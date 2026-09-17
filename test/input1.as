; File: input1.as
; Comprehensive valid input test file

.entry MAIN
.entry DATA_WORD
.extern EXT_LABEL

mcro SAVE_REGS
    sw $1, 0, $30
    sw $2, 4, $30
mcroend

MAIN:   addi $1, 10, $2
        SAVE_REGS
        move $3, $2
        and $4, $2, $3
        la EXT_LABEL
        call EXT_LABEL
        beq $3, $4, END_PROG

LOOP:   subi $1, 1, $1
        bgt $1, $0, LOOP

END_PROG: hlt

STR_DATA:  .asciz "Hello, World!"
DATA_BYTE: .db 10, -5, +20
DATA_HALF: .dh 1000, -2000
DATA_WORD: .dw 100000, -200000
