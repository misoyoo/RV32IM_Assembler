# 검증 과정 및 결과

## 1. 검증 개요

최종 코드 검토에서는 PDF 25페이지를 다시 확인하고 다음을 실제 실행했다.

1. `-Wall -Wextra -std=c11`로 어셈블러와 테스트를 컴파일
2. 기존 C 테스트 13개와 추가 C 테스트 3개 실행
3. 13개 assembly fixture의 성공·실패 기대값 확인
4. 교수님 예제 `asm_example1.s`~`asm_example4.s`를 원본 그대로 재어셈블
5. 56개 필수 명령, 필수 가상 명령, immediate 경계 및 지원 범위 확인

이번 검증에서는 CPUlator나 실제 보드에서 실행하지 않았다. 아래의 이전 CPUlator 실행 기록은 최신 검증 결과로 재확인되지 않았으므로 완료된 결과로 간주하지 않는다.

## 2. 빌드 환경 및 명령

프로젝트 루트에서 다음 명령으로 빌드했다.

```bash
gcc -Wall -Wextra -std=c11 -o assembler \
    main.c pass2.c pass1.c parser.c directives.c \
    symbol_table.c encoder.c register.c
```

## 3. 제공 예제 어셈블 검증

`examples/` 폴더의 어셈블리 예제를 원본 그대로 사용하여 `.bin` 및 `.lst`를 생성했다.

```bash
mkdir -p output

for file in examples/*.s; do
    name=$(basename "$file" .s)

    ./assembler "$file" \
        "output/${name}.bin" \
        "output/${name}.lst" || exit 1
done
```

최종 재검증에서 `examples/*.s` 9개를 모두 어셈블했다. 교수님 제공 예제 4개 및 나머지 예제의 생성 Binary 크기는 다음과 같다.

| 예제 | Binary 크기 |
|---|---:|
| `asm_example1.s` | 716 B |
| `asm_example2.s` | 796 B |
| `asm_example3.s` | 884 B |
| `asm_example4.s` | 2,380 B |
| `ex01_getting_started.s` | 92 B |
| `interval_timer.s` | 76 B |
| `jtag_uart_echo.s` | 155 B |
| `seven_segment_display.s` | 368 B |
| `bss_layout_test.s` | 24 B |

아홉 예제 모두 `ASSEMBLY SUCCESS!`로 끝났고 Binary Validation에서 실제 생성 크기와 예상 크기가 일치했다.

Text와 Data 사이의 정렬 간격이 포함될 수 있으므로 Text 크기와 Data 크기의 단순 합이 최종 Binary 크기와 항상 일치하는 것은 아니다. BSS는 Raw Binary에 포함하지 않는다.

## 4. 시작 주소 `0x00010000` 검증

교수님 예제 1~4의 어셈블 로그에서 Text Base가 `0x00010000`으로 출력됨을 확인했다. 이 검사는 바이너리의 시뮬레이터 실행을 뜻하지 않는다.

사용한 명령은 다음과 같다.

```bash
for i in 1 2 3 4; do
    name="asm_example$i"

    ./assembler "examples/$name.s" \
        "output/$name.bin" \
        "output/$name.lst" || break

    head -n 3 "output/$name.lst"
done
```

실제 실행 결과는 다음과 같다.

| 예제 | Text Base | Listing 첫 명령어 주소 | Binary 크기 |
|---|---|---|---:|
| `asm_example1.s` | `0x00010000` | `00010000` | 716 B |
| `asm_example2.s` | `0x00010000` | `00010000` | 796 B |
| `asm_example3.s` | `0x00010000` | `00010000` | 884 B |
| `asm_example4.s` | `0x00010000` | `00010000` | 2,380 B |

네 가지 예제 모두 Text Base가 `0x00010000`으로 출력되었으며, Listing의 첫 번째 명령어 주소도 `00010000`으로 확인되었다.

따라서 어셈블러의 기본 Text 시작 주소 설정과 주소 배치가 정상적으로 동작하는 것을 확인했다.

## 5. BSS 배치 검증

`tests/builtin_layout_test.s` fixture와 `examples/bss_layout_test.s`를 어셈블하고 Binary Validation을 확인했다. 또한 Pass 1의 BSS 주소 계산을 코드와 대조했다. 이 검증에서는 주소 배치 규칙을 확인했으며, 아래 표는 개별 심볼 주소나 BSS 크기 측정값을 제시하지 않는다.

| 항목 | 결과 |
|---|---|
| Text 시작 주소 | `0x00010000` |
| Data 시작 | Text 끝의 다음 16-byte 경계 |
| BSS 시작 | Data 끝의 다음 16-byte 경계 |
| Binary 출력 | BSS 바이트 제외 |

BSS는 Data 영역 뒤에서 시작하며, Raw Binary에 초기화 데이터로 포함되지 않고 주소와 크기 및 심볼 정보로 관리된다.

## 6. 오류 검증 및 오류 처리 결과

오류 fixture 10개를 실행해 잘못된 입력이 모두 거부되는지 확인했다.

| 테스트 범주 | 관찰 결과 |
|---|---|
| 알 수 없는 명령·잘못된 operand 개수 | 어셈블 실패 |
| 중복 라벨·미정의 라벨 | 어셈블 실패 |
| 잘못된 레지스터 | 어셈블 실패 |
| 홀수·범위 초과 branch/jump | 어셈블 실패 |
| BSS에 데이터 값 지시어 | 어셈블 실패 |
| `sret`, `fence.i`, `uret` | Pass 1에서 거부 |
| signed 범위 초과 `addi` 양수 | Pass 2에서 거부, `.bin` 미생성 |

### 6.1 구현 및 검증 과정에서 확인한 문제와 처리

**① BSS와 Raw Binary 출력 범위**

BSS는 초기화 데이터가 없는 공간이므로 Data처럼 바이너리 바이트를 생성하면 안 된다.

이를 해결하기 위해 Pass 1에서는 BSS의 크기와 라벨 주소를 별도로 계산하고, Pass 2에서는 BSS 영역을 Raw Binary 출력 대상에서 제외하도록 구성했다.

`bss_layout_test.s`를 사용해 BSS 크기와 심볼 주소가 올바르게 계산되는지 확인했다.

**② Text와 Data 사이의 정렬**

Text 영역이 끝난 직후 Data를 무조건 배치하면 16바이트 정렬 조건을 충족하지 못할 수 있다.

따라서 Data Base를 Text 끝 주소의 다음 16바이트 경계로 계산하도록 구성했다. Pass 2에서도 정렬 공간을 바이너리 배치에 반영하고, 생성 크기 검증을 수행하도록 했다.

**③ Raw Binary 로드 주소와 PC**

Raw Binary를 `0x00010000`에 정상적으로 로드했더라도 CPUlator의 PC가 자동으로 같은 주소로 변경되지는 않았다.

Raw Binary에는 시작 주소 메타데이터가 없다. 실행 환경에서 로드 주소와 PC를 별도로 설정해야 한다. 이번 최종 검증에서는 CPUlator 실행을 하지 않았다.

**④ 잘못된 입력에 대한 오류 처리**

잘못된 명령어, 중복 라벨, 정의되지 않은 심볼, 잘못된 레지스터 및 immediate, 잘못된 BSS 지시어를 별도 테스트로 재현했다.

각 입력이 Pass 1 또는 Pass 2에서 실패하는지 확인했다.

잘못된 immediate 및 BSS 지시어 테스트에서는 기존 출력 파일을 먼저 삭제한 후 재실행했고, 실패 실행에서 새로운 `.bin` 및 `.lst` 파일이 생성되지 않는 것을 확인했다.

현재 인코딩 관련 오류 메시지는 mnemonic 중심으로 출력되며, 모든 잘못된 operand에 대해 상세한 원인을 표시하는 기능까지 구현된 것은 아니다.

## 7. CPUlator 실행 검증 상태

CPUlator 실행은 이번 최종 재검증에서 수행하지 않았다. 아래 절차와 기능별 설명은 재현·코드 검토 참고이며 예제 동작 검증 결과가 아니다.

공통 설정 안내:

- Format: Raw
- Element size: 1 byte
- Start address: `0x00010000`
- PC: `0x00010000`

네 프로그램 모두 어셈블러의 Text Base가 `0x00010000`이며 Listing 첫 명령어 주소도 `00010000`임을 확인했다. CPUlator에서 실행하지 않았다.

### 7.1 예제 1: 스톱워치

주요 기능:

- 10ms 주기 타이머 인터럽트
- KEY0 시작 및 정지
- KEY1 초기화
- HEX5~HEX0에 분, 초, 1/100초 표시
- LEDR0 상태 표시

시간 증가, 시작·정지 입력, 초기화 동작은 CPUlator에서 확인하지 않았다.

카운터 범위는 다음과 같다.

| 카운터 | 범위 |
|---|---|
| 1/100초 | 00~99 |
| 초 | 00~59 |
| 분 | 00~99 |

### 7.2 예제 2: 반응속도 측정기

주요 기능:

- mtime 하위 32비트를 이용한 임의 지연
- 1,000~3,999ms 지연
- LEDR9 점등
- KEY0 입력 시점까지 반응 시간 측정
- HEX 및 JTAG UART에 결과 출력
- LED 점등 전 입력 시 FAIL 출력

`FAIL` 처리와 측정값 출력은 소스의 구현 의도 설명이며 CPUlator 동작으로 확인된 결과가 아니다.

소스 코드에서는 다음과 같은 계산을 사용한다.

```text
delay_ms = 1000 + (mtime_low32 % 3000)
delay_ticks = delay_ms × 50000
```

mtime 하위 32비트를 읽어 나머지 연산으로 지연값을 계산하는 내부 구현은 소스 코드로 확인했다.

### 7.3 예제 3: VGA 공 튀기기

주요 기능:

- 160×120 RGB565 VGA 화면
- 8×8 사각형 이동 및 벽 충돌
- SW2~SW0에 따른 속도 조절
- KEY0에 따른 색상 변경
- KEY1에 따른 튕긴 횟수 초기화
- 문자 버퍼에 튕긴 횟수와 속도 출력
- 더블 버퍼링 및 VSync

소스 코드에서 확인한 속도는 다음과 같다.

```text
speed = (SW & 0x7) + 1
```

따라서 프레임당 이동 거리는 1~8픽셀이다.

사각형 이동, 색상·속도 변경, 튕긴 횟수 표시 및 VSync 동작은 이번 최종 검증에서 CPUlator로 확인하지 않았다.

더블 버퍼링은 두 픽셀 버퍼 주소를 교대로 사용하고, 이전 공의 위치를 버퍼별로 관리하는 방식으로 구현했다.

VSync는 `WAIT_VSYNC`에서 Buffer swap을 요청한 뒤 Pixel Buffer Controller의 Status.S 비트를 확인하여 완료될 때까지 대기하는 방식으로 구현했다.

### 7.4 예제 4: JTAG UART 명령 해석기

주요 기능:

| 명령어 | 동작 |
|---|---|
| `HELP` | 명령어 목록 출력 |
| `LED 0x155` | LEDR8~0에 지정한 패턴 출력 |
| `HEX 1234` | HEX5~HEX0에 16진수 표시 |
| `SW` | 현재 SW9~SW0 값을 16진수 및 10진수로 출력 |
| `TIMER 500` | 500ms 주기로 LEDR9 토글 |
| `TIMER 0` | 타이머 토글 중지 |

UART 수신은 인터럽트가 아니라 폴링 방식으로 구현했다.

`GET_JTAG`에서 JTAG UART Data 레지스터의 RVALID 비트를 검사하고, 수신된 문자가 있으면 하위 8비트를 반환한다.

`READ_LINE`에서는 Enter가 입력될 때까지 한 줄을 수신하며, 문자 에코 및 백스페이스를 지원한다.

타이머는 별도의 interval timer IRQ 16으로 처리한다. 따라서 UART 수신 방식은 폴링이고 타이머 동작은 인터럽트 방식이다.

명령별 동작은 소스 코드에서 읽을 수 있으나, 이번 최종 검증에서 UART 입출력이나 타이머 동작을 시뮬레이터에서 확인하지 않았다.

## 8. 최종 검증 요약

| 검증 항목 | 결과 |
|---|---|
| 경고 없는 `-Wall -Wextra -std=c11` 빌드 | 성공 |
| 기존 C 테스트 13개 + 추가 C 테스트 3개 | 16/16 통과 |
| Assembly fixture | 13/13 기대 결과 일치 |
| 교수님 예제 1~4 어셈블 및 Binary Validation | 성공 |
| 필수 56개 명령 smoke 입력 | 224바이트 생성 |
| 필수 pseudo smoke 입력 | 96바이트 생성 |
| Immediate·시스템 명령 경계 입력 | 기대한 허용/거부와 일치 |
| CSR 이름 범위 및 숫자 주소 | 지정 9개 허용, 추가 이름 거부, 숫자 주소 허용 |
| CPUlator 실행 | 미검증 |
| 실제 DE10-Lite 보드 실행 | 미검증 |

56개 명령 입력과 pseudo smoke 입력은 정상 어셈블됐지만, 모든 명령어 결과 워드를 독립 oracle과 비교하거나 CPU에서 실행한 것은 아니다. 대표 인코딩과 형식별 경계 테스트만 별도로 확인했다.
