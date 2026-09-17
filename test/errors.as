; File: errors.as
; Invalid test file for Pass 1 & Pass 2 syntax checking

mcro MY_MCRO
    move $1, $2
mcroend

; 1. Invalid label syntax (starts with a digit)
1LABEL: add $1, $2, $3

; 2. Duplicate label definition
LABEL1: add $1, $2, $3
LABEL1: sub $1, $2, $3

; 3. Too few operands for R-type instruction
        add $1, $2

; 4. Too many operands for J-type instruction
        hlt $1

; 5. Invalid register format / out of range
        addi $35, 10, $2
        move $1, $05

; 6. Consecutive commas error
        ori $1,, 5, $2

; 7. Trailing comma error
        .db 1, 2, 3,

; 8. Value out of range for byte directive (.db allows [-128, 127])
        .db 500

; 9. Label defined as both .entry and .extern
.entry DUAL_SYM
.extern DUAL_SYM
DUAL_SYM: add $1, $2, $3
