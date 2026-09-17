; File: input2.as
; Valid test file focusing on memory access and registers

.entry START
.extern EXTERNAL_FUNC

mcro CLEAR_REG
    add $0, $0, $5
mcroend

START:  lh $10, 0, $1
        lw $11, 4, $1
        sb $10, 8, $2
        sw $11, 12, $2
        
        CLEAR_REG
        
        blt $10, $11, TARGET
        jmp $31

TARGET: call EXTERNAL_FUNC
        jmp TARGET_REG
        
TARGET_REG: jmp $5
            hlt

TEXT:  .asciz "Assembly Language Project 2026"
ARRAY: .dw 1, 2, 3, 4, 5
