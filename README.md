# RV32IM Assembler

C로 구현한 RISC-V RV32IM 2-pass 어셈블러입니다. 어셈블리 소스(`.s`)를 읽어 CPUlator RISC-V DE10-Lite에서 실행할 수 있는 little-endian Raw Binary(`.bin`)와 Listing(`.lst`)을 생성합니다.

출력은 ELF가 아닌 Raw Binary입니다. 파일 자체에는 시작 주소나 relocation 정보가 포함되지 않습니다.

## 주요 기능

- **Two-pass 구조**
  - Pass 1: 섹션 크기, 메모리 주소 및 심볼 정보를 계산합니다.
  - Pass 2: 명령어를 기계어로 인코딩하고 초기화 데이터를 바이너리에 기록합니다.
- **명령어 인코딩**: RV32I, RV32M, CSR 및 System 명령어와 R/I/S/B/U/J 형식을 지원합니다.
- **레지스터**: `x0`~`x31`, ABI 레지스터 이름 및 `fp`(`s0`) 별칭을 지원합니다.
- **심볼 처리**: 라벨, `.equ`, `.global`, `.globl` 및 내장 심볼 5개를 지원합니다.
- **의사 명령어**: `li`, `la`, `mv`, `nop`, `not`, `neg`, `j`, `jr`, `call`, `ret`, 조건 분기 별칭 및 CSR 별칭 등을 지원합니다.
- **데이터 및 정렬**: `.text`, `.data`, `.bss`, `.byte`, `.half`, `.word`, `.ascii`, `.asciz`, `.space`, `.align`을 지원합니다.
- **오류 검사**: immediate 범위, 분기·점프 거리, 잘못된 레지스터, operand 개수, 중복 라벨 및 미정의 심볼 등을 검사합니다.
- **Binary Validation**: Pass 1에서 계산한 예상 크기와 실제 생성된 바이너리 크기를 비교합니다.

## 폴더 구성

```text
RV32IM_Assembler/
├── main.c
├── parser.c / parser.h
├── register.c / register.h
├── encoder.c / encoder.h
├── directives.c / directives.h
├── symbol_table.c / symbol_table.h
├── pass1.c / pass1.h
├── pass2.c / pass2.h
├── examples/                 # 어셈블리 예제 9개
├── tests/                    # C 테스트 및 검증용 소스
├── docs/
│   ├── PROCESS.md            # 구현 과정 및 동작 상세
│   ├── RESULTS.md            # 검증 과정 및 결과
│   └── AI_PROMPTS.md         # AI 사용 및 프롬프트 기록
├── output/                   # 생성되는 .bin 및 .lst
├── README.md
└── .gitignore
```

## 빌드

프로젝트 루트 디렉터리에서 실행합니다.

```bash
gcc -Wall -Wextra -std=c11 -o assembler \
    main.c pass2.c pass1.c parser.c directives.c \
    symbol_table.c encoder.c register.c
```

## 실행

```bash
./assembler <source_file> <binary_file> <listing_file>
```

예시:

```bash
mkdir -p output

./assembler examples/asm_example1.s \
    output/asm_example1.bin output/asm_example1.lst
```

정상적으로 처리되면 Pass 1, Pass 2 및 Binary Validation을 수행하고 `ASSEMBLY SUCCESS!`를 출력합니다.

## 메모리 배치

기본 `.text` 시작 주소는 `0x00010000`입니다.

| 심볼 | 주소 계산 |
|---|---|
| `TEXT_START` | `0x00010000` |
| `_data_start` | Text 끝에서 다음 16-byte 경계 |
| `_bss_start` | Data 끝에서 다음 16-byte 경계 |
| `_bss_end` | BSS 끝 주소 |
| `_stack_top` | `0x00040000` |

주소 계산은 다음과 같습니다.

```text
data_base = align_up(text_base + text_size, 16)
bss_base  = align_up(data_base + data_size, 16)
bss_end   = bss_base + bss_size
```

위 5개 심볼은 어셈블러가 미리 정의합니다. 첫 섹션 지시어가 생략된 경우 `.text`에서 시작합니다.

Raw Binary에는 `.text`, `.data` 및 영역 사이의 정렬 패딩이 포함됩니다. `.bss`는 주소와 크기만 계산하며 바이너리에는 기록하지 않습니다. `.bss`에서는 `.space`와 `.align`을 지원합니다.

## `jal` 기본 목적지 레지스터 관련 사항

한 operand 형식의 `jal label`은 `jal ra, label`로 인코딩합니다. 제공된 과제 예제들이 `jal label`로 함수를 호출하고 `ret`으로 복귀하므로, 원본 예제를 수정하지 않고 실행하기 위한 선택입니다.

과제 PDF 11페이지에는 `jal label`을 `jal a0, label`로 치환한다고 기재되어 있어 해당 규칙과 차이가 있습니다. 목적 레지스터를 명시하는 `jal rd, label` 형식은 지원하므로 `jal a0, label`도 직접 사용할 수 있습니다.

`call label`은 `jal ra, label` 한 개로 확장하며, `ret`은 `jalr x0, 0(ra)`로 확장합니다.

## 제공 예제

`examples/`에는 다음 9개 예제가 포함됩니다.

| 예제 | 주요 기능 |
|---|---|
| `asm_example1.s` | 인터럽트 기반 스톱워치 |
| `asm_example2.s` | 반응속도 측정기 |
| `asm_example3.s` | VGA 공 튀기기 |
| `asm_example4.s` | JTAG UART 명령 해석기 |
| `ex01_getting_started.s` | 스위치 및 LED 제어 |
| `interval_timer.s` | 인터벌 타이머 |
| `jtag_uart_echo.s` | JTAG UART 문자 에코 |
| `seven_segment_display.s` | 7세그먼트 디스플레이 |
| `bss_layout_test.s` | 메모리 배치 및 BSS 검증 |

전체 예제를 어셈블하려면 다음 명령을 사용합니다.

```bash
for file in examples/*.s; do
    name=$(basename "$file" .s)

    ./assembler "$file" \
        "output/${name}.bin" \
        "output/${name}.lst" || exit 1
done
```

## C 테스트

`tests/`의 12개 C 테스트는 다음과 같이 컴파일하고 실행할 수 있습니다.

```bash
test_dir=$(mktemp -d)

for file in tests/*.c; do
    name=$(basename "$file" .c)
    exe="$test_dir/$name"

    gcc -Wall -Wextra -std=c11 -I. \
        "$file" \
        pass2.c pass1.c parser.c directives.c \
        symbol_table.c encoder.c register.c \
        -o "$exe" || exit 1

    case "$name" in
        pass1_file_test)
            "$exe" examples/asm_example1.s || exit 1
            ;;
        pass2_file_test)
            "$exe" examples/asm_example1.s \
                "$test_dir/test.bin" "$test_dir/test.lst" || exit 1
            ;;
        *)
            "$exe" || exit 1
            ;;
    esac
done

rm -rf "$test_dir"
```

## CPUlator에서 실행

대상 환경은 CPUlator RISC-V DE10-Lite(`rv32-de10-lite`)입니다.

1. 어셈블러로 `.bin` 파일을 생성합니다.
2. CPUlator에서 **File → Load/Save Memory**를 엽니다.
3. Format을 `Raw`, Element size를 `1 byte`로 설정합니다.
4. Start address에 `10000`(16진수 `0x00010000`)을 입력하고 바이너리를 로드합니다.
5. Disassembly에서 주소 `00010000`의 첫 명령어를 확인합니다.
6. PC를 `00010000`으로 설정한 뒤 실행합니다.

Raw Binary에는 시작 주소 메타데이터가 없으므로 메모리 로드 주소와 PC 설정은 별도로 진행해야 합니다.

## 문서

- [구현 과정 및 동작 상세](docs/PROCESS.md)
- [검증 과정 및 결과](docs/RESULTS.md)
- [AI 사용 및 프롬프트 기록](docs/AI_PROMPTS.md)

## 제한 사항 및 AI 사용 고지

- ELF 생성, 링커, relocation 및 여러 오브젝트 파일 링크는 지원하지 않습니다.
- BSS는 Raw Binary에 포함하지 않습니다.
- 일부 인코딩 오류는 mnemonic 중심으로 출력하며 모든 operand에 대한 상세 진단을 제공하지는 않습니다.
- `jal label` 기본 목적지 레지스터는 제공 예제 호환을 위해 `ra`를 사용하며, 과제 PDF의 `a0` 기본 규칙과 차이가 있습니다.
- 검증은 CPUlator 시뮬레이터에서 수행했으며 실제 DE10-Lite 보드에서 수행한 것은 아닙니다.

본 프로젝트의 설계, 코드 작성·수정, 오류 분석 및 테스트 과정에서 생성형 AI를 사용했습니다. 대표적인 프롬프트와 AI 및 사용자 역할은 [AI 사용 및 프롬프트 기록](docs/AI_PROMPTS.md)에 정리했습니다.
