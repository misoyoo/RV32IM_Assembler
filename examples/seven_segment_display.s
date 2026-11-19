.data
seg7:                                               # 7세그먼트 숫자 패턴 배열
    .byte 0x3F, 0x06, 0x5B, 0x4F                    # 0, 1, 2, 3
    .byte 0x66, 0x6D, 0x7D, 0x07                    # 4, 5, 6, 7
    .byte 0x7F, 0x6F, 0x77, 0x7C                    # 8, 9, A, b
    .byte 0x39, 0x5E, 0x79, 0x71                    # C, d, E, F

.text
.global _start

_start:
    li s0, 0xFF200040                                  # SW의 메모리 주소
    li s1, 0xFF200020                                  # HEX3~HEX0의 메모리 주소
    li s2, 0xFF200030                                  # HEX5~HEX4의 메모리 주소
    la s6, seg7                                        # seg7 배열의 시작 주소

    lw t0, 0(s0)                                       # 스위치 전체 값 읽기
    srli t1, t0, 8                                     # SW8을 bit 0으로 이동
    andi t1, t1, 1                                     # SW8 값만 추출

    beqz t1, init_up                                   # SW8이 0이면 UP 초기화
    li s3, 99                                          # DOWN이면 count = 99
    j init_done                                        # 초기화 완료로 이동

init_up:
    li s3, 0                                           # UP이면 count = 0

init_done:
    li t2, 10                                          # 십의 자리 계산을 위한 10
    divu s4, s3, t2                                    # prev_tens = count / 10
    li s5, 0                                           # under_on = 0

main_loop:
    lw t0, 0(s0)                                       # 현재 스위치 값 읽기
    srli t1, t0, 8                                     # SW8을 bit 0으로 이동
    andi t1, t1, 1                                     # SW8 값 추출

    li t6, 10                                           # 나눗셈 기준값 10
    divu t2, s3, t6                                     # tens = count / 10
    remu t3, s3, t6                                     # ones = count % 10

    bne t2, s4, tens_changed                            # 십의 자리가 바뀌었는지 확인
    j display_count                                     # 바뀌지 않았으면 출력으로 이동

tens_changed:
    xori s5, s5, 1                                      # under_on 값을 0과 1로 반전
    mv s4, t2                                           # prev_tens = tens

display_count:
    add t4, s6, t2                                      # seg7[tens]의 주소 계산
    lbu t4, 0(t4)                                       # HEX5에 표시할 패턴 읽기

    add t5, s6, t3                                      # seg7[ones]의 주소 계산
    lbu t5, 0(t5)                                       # HEX4에 표시할 패턴 읽기

    slli t4, t4, 8                                      # HEX5 패턴을 8비트 왼쪽 이동
    or t4, t4, t5                                       # HEX5와 HEX4 패턴 결합
    sw t4, 0(s2)                                        # HEX5-4에 출력

    slli t4, s5, 3                                      # under_on을 3비트 왼쪽 이동
                                                        # 1이면 0x08, 0이면 0x00

    srli a3, t0, 9                                      # SW9를 bit 0으로 이동
    andi a3, a3, 1                                      # SW9 값 추출
    bnez a3, decimal_mode                               # SW9가 1이면 10진수 출력

hex_mode:
    li a0, 0                                            # HEX2를 끄기 위한 값

    srli a3, t0, 4                                      # SW7~SW4를 오른쪽으로 이동
    andi a3, a3, 0x0F                                   # 상위 4비트 추출
    add a3, s6, a3                                      # seg7 배열 주소 계산
    lbu a1, 0(a3)                                       # HEX1 패턴 읽기

    andi a3, t0, 0x0F                                   # SW3~SW0 추출
    add a3, s6, a3                                      # seg7 배열 주소 계산
    lbu a2, 0(a3)                                       # HEX0 패턴 읽기

    j output_hex                                        # 출력 부분으로 이동

decimal_mode:
    andi a3, t0, 0xFF                                   # SW7~SW0 값 추출

    li t6, 100                                          # 백의 자리 계산 기준값
    divu a0, a3, t6                                     # 백의 자리 계산
    remu a3, a3, t6                                     # 나머지 0~99 계산

    li t6, 10                                           # 십의 자리 계산 기준값
    divu a1, a3, t6                                     # 십의 자리 계산
    remu a2, a3, t6                                     # 일의 자리 계산

    add a0, s6, a0                                      # seg7[백의 자리] 주소
    lbu a0, 0(a0)                                       # HEX2 패턴 읽기

    add a1, s6, a1                                      # seg7[십의 자리] 주소
    lbu a1, 0(a1)                                       # HEX1 패턴 읽기

    add a2, s6, a2                                      # seg7[일의 자리] 주소
    lbu a2, 0(a2)                                       # HEX0 패턴 읽기

output_hex:
    slli t4, t4, 24                                     # HEX3 패턴을 24비트 이동
    slli a0, a0, 16                                     # HEX2 패턴을 16비트 이동
    slli a1, a1, 8                                      # HEX1 패턴을 8비트 이동

    or t4, t4, a0                                       # HEX3과 HEX2 결합
    or t4, t4, a1                                       # HEX1 추가
    or t4, t4, a2                                       # HEX0 추가

    sw t4, 0(s1)                                        # HEX3~HEX0에 출력

    li t6, 200000                                       # 딜레이 반복 횟수
delay_loop:
    addi t6, t6, -1                                     # 딜레이 변수 1 감소
    bnez t6, delay_loop                                 # 0이 아니면 계속 반복

    lw t0, 0(s0)                                        # 최신 스위치 값 다시 읽기
    srli t1, t0, 8                                      # SW8을 bit 0으로 이동
    andi t1, t1, 1                                      # SW8 값 추출

    bnez t1, count_down                                 # SW8이 1이면 DOWN으로 이동

count_up:
    li t2, 99                                           # 최대 카운터 값 99
    bge s3, t2, up_reset                                # count가 99 이상이면 초기화
    addi s3, s3, 1                                      # count = count + 1
    j main_loop                                         # 다시 메인 반복문으로 이동

up_reset:
    li s3, 0                                            # count = 0
    j main_loop                                         # 다시 메인 반복문으로 이동

count_down:
    beqz s3, down_reset                                 # count가 0이면 99로 초기화
    addi s3, s3, -1                                     # count = count - 1
    j main_loop                                         # 다시 메인 반복문으로 이동

down_reset:
    li s3, 99                                           # count = 99
    j main_loop                                         # 다시 메인 반복문으로 이동