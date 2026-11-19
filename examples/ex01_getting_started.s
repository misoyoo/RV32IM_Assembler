# 예제 1 (어셈블리 핵심): SW -> LEDR 회전 패턴 (원문 Listing 3 대응)
# RISC-V에는 ROR 명령이 없으므로 slli/srli/or 로 10비트 회전을 구현한다
.equ LEDR_BASE, 0xFF200000
.equ SW_BASE, 0xFF200040
.equ KEY_BASE, 0xFF200050
.text
.global _start
_start:
li s0, LEDR_BASE # LED 포트 주소
li s1, SW_BASE # SW 포트 주소
li s2, KEY_BASE # KEY 포트 주소
li s3, 0x00F # 초기 패턴
DO_DISPLAY:
lw t0, 0(s2) # KEY 읽기
beqz t0, NO_BUTTON # 눌린 키가 없으면 건너뜀
lw s3, 0(s1) # SW 값을 새 패턴으로
andi s3, s3, 0x3FF
WAIT: lw t0, 0(s2)
bnez t0, WAIT # 키를 뗄 때까지 대기
NO_BUTTON:
sw s3, 0(s0) # LEDR9-0 에 패턴 출력
slli t1, s3, 1 # 10비트 왼쪽 회전: (p<<1 | p>>9) & 0x3FF
srli t2, s3, 9
or s3, t1, t2
andi s3, s3, 0x3FF
li t3, 200000 # 지연 루프
DELAY: addi t3, t3, -1
bnez t3, DELAY
j DO_DISPLAY