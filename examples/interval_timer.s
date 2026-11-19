
.equ LEDR_BASE, 0xFF200000
.equ TIMER_BASE, 0xFF202000

.text
.global _start

_start:
    li s0, LEDR_BASE
    li s1, TIMER_BASE

    li t0, 50000000
    sw t0, 8(s1)          # 시작값 하위 16비트
    srli t0, t0, 16
    sw t0, 12(s1)         # 시작값 상위 16비트

    li t0, 0x6            # START | CONT
    sw t0, 4(s1)          # Control 레지스터

    li s2, 1              # LED 값

LOOP:
    sw s2, 0(s0)          # LEDR 출력

WAIT:
    lw t1, 0(s1)          # Status 레지스터 읽기
    andi t1, t1, 1        # TO 비트 확인
    beqz t1, WAIT         # TO=0이면 계속 대기

    sw zero, 0(s1)        # TO 비트 클리어

    xori s2, s2, 1        # LED 값 토글
    j LOOP
