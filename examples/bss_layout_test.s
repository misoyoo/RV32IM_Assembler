.text
.global _start

_start:
    addi zero, zero, 0

.data
name:
    .asciz "ab"
    .align 2

value:
    .word 42

.bss
    .align 4

stack:
    .space 1024

tcb:
    .space 64