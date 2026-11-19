
.equ JTAG_UART_BASE, 0xFF201000
.equ STACK_TOP, 0x04000000

.text
.global _start

_start:
    li sp, STACK_TOP
    la s0, TEXT_STRING

LOOP:
    lbu a0, 0(s0)          # 문자열에서 문자 읽기
    beqz a0, CONT          # NULL 문자면 문자열 출력 종료

    jal PUT_JTAG           # 문자 전송
    addi s0, s0, 1         # 다음 문자로 이동
    j LOOP

CONT:
    jal GET_JTAG           # 수신 문자 -> a0
    beqz a0, CONT          # 받은 문자가 없으면 계속 대기

    jal PUT_JTAG           # 수신 문자 에코
    j CONT

# -------------------------
# PUT_JTAG
# 입력: a0 = 전송할 문자
# -------------------------
PUT_JTAG:
    li t0, JTAG_UART_BASE
    lw t1, 4(t0)           # Control 레지스터

    srli t1, t1, 16        # WSPACE 추출
    beqz t1, END_PUT       # FIFO가 가득 차면 전송하지 않음

    sw a0, 0(t0)           # Data 레지스터에 문자 쓰기

END_PUT:
    ret

# -------------------------
# GET_JTAG
# 출력: a0 = 수신 문자 (없으면 0)
# -------------------------
GET_JTAG:
    li t0, JTAG_UART_BASE
    lw a0, 0(t0)           # Data 레지스터 읽기

    li t1, 0x8000
    and t1, a0, t1         # RVALID 확인
    beqz t1, RET_NULL      # RVALID=0이면 문자 없음

    andi a0, a0, 0xFF      # DATA 필드 추출
    ret

RET_NULL:
    li a0, 0
    ret

.data
TEXT_STRING:
.asciz "\nJTAG UART example code\n> "
