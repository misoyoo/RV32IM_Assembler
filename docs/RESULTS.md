# 검증 과정 및 결과

## 1. 검증 개요

본 프로젝트의 검증은 다음 순서로 진행했다.

1. macOS 환경에서 어셈블러 컴파일
2. 제공된 어셈블리 예제를 원본 그대로 어셈블
3. Pass 1 및 Pass 2 실행 결과 확인
4. Binary Validation을 통한 바이너리 크기 검증
5. Listing 첫 명령어 주소와 섹션 배치 확인
6. BSS 전용 예제 검증
7. 잘못된 입력에 대한 오류 처리 검증
8. CPUlator RISC-V DE10-Lite 시뮬레이터에서 예제 1~4 실행

CPUlator 검증은 실제 DE10-Lite 보드가 아닌 웹 기반 시뮬레이터에서 수행했다.

## 2. 빌드 환경 및 명령

프로젝트 루트에서 다음 명령으로 빌드했다.

```bash
gcc -Wall -Wextra -std=c11 -o assembler \
    main.c pass2.c pass1.c parser.c directives.c \
    symbol_table.c encoder.c register.c
```

## 3. 제공 예제 전체 어셈블 검증

`examples/` 폴더의 어셈블리 예제를 원본 그대로 사용하여 `.bin` 및 `.lst`를 생성했다.

```bash
mkdir -p output

for file in examples/*.s; do
    name=$(basename "$file" .s)

    ./assembler "$file" \
        "output/${name}.bin" \
        "output/${name}.lst" || break
done
```

마지막 전체 실행에서 다음 9개 예제 모두 `ASSEMBLY SUCCESS!`로 종료되었다.

| 예제 | Text 크기 | Data 시작 주소 | Data 크기 | Binary 크기 |
|---|---:|---:|---:|---:|
| `asm_example1.s` | 680 B | `0x000102B0` | 28 B | 716 B |
| `asm_example2.s` | 664 B | `0x000102A0` | 124 B | 796 B |
| `asm_example3.s` | 800 B | `0x00010320` | 84 B | 884 B |
| `asm_example4.s` | 1,916 B | `0x00010780` | 460 B | 2,380 B |
| `ex01_getting_started.s` | 92 B | - | 0 B | 92 B |
| `interval_timer.s` | 76 B | - | 0 B | 76 B |
| `jtag_uart_echo.s` | 124 B | `0x00010080` | 27 B | 155 B |
| `seven_segment_display.s` | 348 B | `0x00010160` | 16 B | 368 B |
| `bss_layout_test.s` | 4 B | `0x00010010` | 8 B | 24 B |

모든 예제에서 실제 생성 크기와 프로그램이 계산한 예상 크기가 일치했다.

Text와 Data 사이의 정렬 간격이 포함될 수 있으므로 Text 크기와 Data 크기의 단순 합이 최종 Binary 크기와 항상 일치하는 것은 아니다. BSS는 Raw Binary에 포함하지 않는다.

## 4. 시작 주소 `0x00010000` 검증

교수 요구사항인 프로그램 시작 주소를 예제 1~4에서 개별적으로 검증했다.

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

`bss_layout_test.s`를 사용하여 `.data`, `.align`, `.bss`, `.space` 처리를 확인했다.

| 항목 | 결과 |
|---|---:|
| Text 시작 주소 | `0x00010000` |
| Text 크기 | 4 B |
| Data 시작 주소 | `0x00010010` |
| Data 크기 | 8 B |
| BSS 시작 주소 | `0x00010018` |
| BSS 크기 | 1,096 B |
| `stack` 심볼 주소 | `0x00010020` |
| `tcb` 심볼 주소 | `0x00010420` |
| Binary 크기 | 24 B |

BSS는 Data 영역 뒤에서 시작하며, `stack`은 BSS 내부의 `.align 4` 적용 후 배치되었다.

BSS는 Raw Binary에 초기화 데이터로 포함되지 않고 주소와 크기 및 심볼 정보로 관리되는 것을 확인했다.

## 6. 오류 검증 및 오류 처리 결과

정상 입력뿐만 아니라 잘못된 입력을 별도로 만들어 Pass 1 및 Pass 2의 오류 처리 여부를 확인했다.

| 테스트 입력 | 관찰된 결과 | 실패 단계 |
|---|---|---|
| 존재하지 않는 명령어 `invalid_op` | `unknown or invalid instruction 'invalid_op'` | Pass 1 |
| `_start` 라벨 중복 정의 | `duplicate or invalid label '_start'` | Pass 1 |
| 정의되지 않은 `MISSING_LABEL`을 `jal`에서 참조 | `cannot encode 'jal'` | Pass 2 |
| 잘못된 레지스터 `badreg` 사용 | `cannot encode 'addi'` | Pass 2 |
| `addi` immediate로 4096 사용 | `cannot encode 'addi'` | Pass 2 |
| BSS에서 `.word 123` 사용 | `only .space and .align are supported in .bss` | Pass 1 |

### 6.1 구현 및 검증 과정에서 확인한 문제와 처리

**① BSS와 Raw Binary 출력 범위**

BSS는 초기화 데이터가 없는 공간이므로 Data처럼 바이너리 바이트를 생성하면 안 된다.

이를 해결하기 위해 Pass 1에서는 BSS의 크기와 라벨 주소를 별도로 계산하고, Pass 2에서는 BSS 영역을 Raw Binary 출력 대상에서 제외하도록 구성했다.

`bss_layout_test.s`를 사용해 BSS 크기와 심볼 주소가 올바르게 계산되는지 확인했다.

**② Text와 Data 사이의 정렬**

Text 영역이 끝난 직후 Data를 무조건 배치하면 16바이트 정렬 조건을 충족하지 못할 수 있다.

따라서 Data Base를 Text 끝 주소의 다음 16바이트 경계로 계산하도록 구성했다. Pass 2에서도 정렬 공간을 바이너리 배치에 반영하고, 생성 크기 검증을 수행하도록 했다.

**③ CPUlator에서 Raw Binary 로드 후 PC가 0에서 시작하는 문제**

Raw Binary를 `0x00010000`에 정상적으로 로드했더라도 CPUlator의 PC가 자동으로 같은 주소로 변경되지는 않았다.

초기에는 PC가 `0x00000000`에서 실행되어 유효한 프로그램 코드가 아닌 메모리 영역을 읽는 문제가 발생했다.

이를 해결하기 위해 CPUlator Disassembly에서 첫 명령어 주소 `0x00010000`으로 이동해 PC를 해당 주소로 설정한 뒤 실행했다.

이는 어셈블러의 주소 배치 오류가 아니라 Raw Binary 실행 환경에서 시작 주소를 별도로 지정해야 하는 문제였다.

**④ 잘못된 입력에 대한 오류 처리**

잘못된 명령어, 중복 라벨, 정의되지 않은 심볼, 잘못된 레지스터 및 immediate, 잘못된 BSS 지시어를 별도 테스트로 재현했다.

각 입력이 Pass 1 또는 Pass 2에서 실패하는지 확인했다.

잘못된 immediate 및 BSS 지시어 테스트에서는 기존 출력 파일을 먼저 삭제한 후 재실행했고, 실패 실행에서 새로운 `.bin` 및 `.lst` 파일이 생성되지 않는 것을 확인했다.

현재 인코딩 관련 오류 메시지는 mnemonic 중심으로 출력되며, 모든 잘못된 operand에 대해 상세한 원인을 표시하는 기능까지 구현된 것은 아니다.

## 7. CPUlator 실행 검증

공통 설정:

- Format: Raw
- Element size: 1 byte
- Start address: `0x00010000`
- PC: `0x00010000`

네 프로그램 모두 어셈블러의 Text Base와 Listing 첫 명령어 주소가 `0x00010000`인 것을 확인한 뒤 실행했다.

### 7.1 예제 1: 스톱워치

주요 기능:

- 10ms 주기 타이머 인터럽트
- KEY0 시작 및 정지
- KEY1 초기화
- HEX5~HEX0에 분, 초, 1/100초 표시
- LEDR0 상태 표시

CPUlator 실행 영상에서 시간 증가, 시작·정지 입력 및 초기화 동작을 확인했다.

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

CPUlator 영상에서 너무 일찍 KEY0를 누른 경우 `FAIL`이 출력되는 것을 확인했다.

정상 측정에서는 HEX와 JTAG UART에 `2903 ms`가 출력되는 것을 확인했다.

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

CPUlator 영상에서 사각형 이동, 색상 변경, 속도 변경 및 문자 버퍼의 튕긴 횟수 표시를 확인했다.

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

`LED 0`은 LEDR8~0을 끄는 명령이다. `TIMER 0`은 타이머를 중지하지만 LEDR9의 현재 출력 상태를 강제로 끄지는 않는다.

## 8. 최종 검증 요약

| 검증 항목 | 결과 |
|---|---|
| 제공 예제 9개 어셈블 | 성공 |
| Pass 1 및 Pass 2 | 성공 |
| Binary Validation | 성공 |
| 예제 1~4 시작 주소 `0x10000` | 확인 |
| Listing 첫 명령어 주소 | 확인 |
| BSS 주소 및 크기 계산 | 확인 |
| 잘못된 입력 6종 오류 검출 | 확인 |
| 예제 1 스톱워치 | CPUlator 동작 확인 |
| 예제 2 반응속도 측정기 | CPUlator 동작 확인 |
| 예제 3 VGA 공 튀기기 | CPUlator 동작 확인 |
| 예제 4 JTAG UART 명령 해석기 | CPUlator 실행 및 소스 구현 확인 |

본 검증은 CPUlator RISC-V DE10-Lite 시뮬레이터 환경에서 진행했으며 실제 DE10-Lite 보드에서 수행한 것은 아니다.

또한 영상은 외부 장치의 출력과 프로그램의 기능 동작을 확인하는 자료이며, mtime 연산, 폴링, 더블 버퍼링 및 VSync와 같은 내부 구현은 소스 코드와 함께 검증했다.