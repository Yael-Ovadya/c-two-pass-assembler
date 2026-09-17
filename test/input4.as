; File: input4.as
.entry BRANCH_TEST

mcro JUMP_MACRO
    jmp $10
mcroend

BRANCH_TEST: move $1, $2
             mvhi $3, $4
             mvlo $5, $6
             beq $1, $2, TARGET1
             bne $3, $4, TARGET2

TARGET1:     bgt $5, $6, BRANCH_TEST
TARGET2:     blt $1, $5, BRANCH_TEST
             JUMP_MACRO
             hlt

VALS: .dh 12, -24, 36, -48
