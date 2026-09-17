; File: input3.as
.entry MATH_START
.extern GLOBAL_VAR

mcro CALCULATE
    add $1, $2, $3
    sub $4, $5, $6
    and $7, $8, $9
mcroend

MATH_START: ori $10, -15, $11
            nori $12, 100, $13
            CALCULATE
XOR_LOOP:   nor $14, $15, $16
            la GLOBAL_VAR
            call GLOBAL_VAR
            hlt

DATA_LIST: .dw 500, -600, 700
STR_LABEL: .asciz "Math Operations Test"
