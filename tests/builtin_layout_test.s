.globl _start

_start:
    la t0, TEXT_START
    la t1, _data_start
    la t2, _bss_start
    la t3, _bss_end
    li sp, _stack_top

.data
    .word TEXT_START, _data_start, _bss_start
    .word _bss_end, _stack_top
    .byte 0xAA

.bss
    .align 4
buffer:
    .space 16
