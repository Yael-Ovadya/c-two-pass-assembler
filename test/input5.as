; File: input5.as
.entry MEM_TEST
.extern BUFFER_REF

mcro STORE_ALL
    sb $1, 0, $2
    sh $3, 2, $4
    sw $5, 4, $6
mcroend

MEM_TEST: lb $1, 0, $2
          lh $3, 2, $4
          lw $5, 4, $6
          STORE_ALL
          la BUFFER_REF
          hlt

BUFF: .db 1, 2, 3, 4, 5, 6, 7, 8
