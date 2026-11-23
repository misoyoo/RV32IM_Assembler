# RV32IM Assembler

C로 구현한 RV32IM 2-pass 어셈블러입니다. 어셈블리 소스(`.s`)를 읽어 little-endian Raw Binary(`.bin`)와 Listing(`.lst`)을 생성합니다. PDF 요구 범위는 RV32I, M, Zicsr와 일부 machine-mode 시스템 명령입니다.

출력은 ELF가 아닌 Raw Binary입니다. 파일 자체에는 시작 주소나 relocation 정보가 포함되지 않습니다.

## 주요 기능

- **Two-pass 구조**
  - Pass 1: 섹션 크기, 메모리 주소 및 심볼 정보를 계산합니다.
  - Pass 2: 명령어를 기계어로 인코딩하고 초기화 데이터를 바이너리에 기록합니다.
- **기본 명령어 56개**: RV32I 40개, M 확장 8개, Zicsr 및 특권 명령 8개를 지원합니다.
- **인코딩**: R/I/S/B/U/J 형식. signed immediate 범위, 분기·점프 오프셋 및 정렬 조건을 검사합니다.
- **레지스터**: `x0`~`x31`, ABI 레지스터 이름 및 `fp`(`s0`) 별칭을 지원합니다.
- **심볼 처리**: 라벨, `.equ`, `.global`, `.globl` 및 내장 심볼 5개를 지원합니다.
- **PDF 가상 명령어**: `li`, `la`, `mv`, `nop`, `not`, `neg`, `j`, `jr`, `call`, `ret`, `beqz`, `bnez`, `bgt`, `ble`, `bgtu`, `bleu`, `csrr`, `csrw`, `csrsi`, `csrci`.
- **데이터 및 정렬**: `.text`, `.data`, `.bss`, `.byte`, `.half`, `.word`, `.ascii`, `.asciz`, `.space`, `.align`을 지원합니다.
- **CSR**: PDF의 `mstatus`, `mie`, `mtvec`, `mscratch`, `mepc`, `mcause`, `mtval`, `mip`, `mhartid` 이름과 12비트 숫자 CSR 주소를 지원합니다.
- **시스템 명령**: `ecall`, `ebreak`, `mret`, `wfi`, 인자 없는 `fence`를 지원합니다. `sret`, `fence.i`, `uret`은 거부합니다.
- **오류 검사**: 범위 초과 immediate, 분기·점프 거리 및 정렬 오류, 잘못된 레지스터·operand 개수, 중복 라벨, 미정의 심볼, 알 수 없는 명령·지시어 등을 검사합니다.
- **Binary Validation**: Pass 1에서 계산한 예상 크기와 실제 생성된 바이너리 크기를 비교합니다.

### 명령어 목록

- RV32I R-type: `add`, `sub`, `sll`, `slt`, `sltu`, `xor`, `srl`, `sra`, `or`, `and`
- M 확장: `mul`, `mulh`, `mulhsu`, `mulhu`, `div`, `divu`, `rem`, `remu`
- I-type 산술: `addi`, `slti`, `sltiu`, `xori`, `ori`, `andi`, `slli`, `srli`, `srai`
- Load: `lb`, `lh`, `lw`, `lbu`, `lhu`, `jalr`
- Store: `sb`, `sh`, `sw`
- Branch: `beq`, `bne`, `blt`, `bge`, `bltu`, `bgeu`
- U/J: `lui`, `auipc`, `jal`
- CSR: `csrrw`, `csrrs`, `csrrc`, `csrrwi`, `csrrsi`, `csrrci`
- System: `ecall`, `ebreak`, `mret`, `wfi`, 인자 없는 `fence`

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

정수 레지스터는 `x0`~`x31` 또는 ABI 이름(`zero`, `ra`, `sp`, `gp`, `tp`, `t0`~`t6`, `s0`~`s11`, `a0`~`a7`)으로 지정할 수 있습니다. `fp`는 `s0`와 같은 레지스터입니다.

CSR 이름은 PDF의 9개 필수 이름 외에 구현된 named alias(`misa`, `medeleg`, `mideleg`, `mcounteren`, `cycle`, `time`, `instret`)도 인식합니다. 숫자 주소는 0~`0xFFF` 범위에서 받을 수 있습니다.

Raw Binary에는 `.text`, `.data` 및 영역 사이의 정렬 패딩이 포함됩니다. `.bss`는 주소와 크기만 계산하며 바이너리에는 기록하지 않습니다. `.bss`에서는 `.space`와 `.align`을 지원합니다.

## 가상 명령 확장

`jal label`은 `jal ra, label`로 처리합니다. 이는 과제 요구에 따른 동작입니다. 목적 레지스터를 직접 지정하는 `jal rd, label`도 지원합니다. `call label`은 `jal ra, label`, `ret`은 `jalr x0, 0(ra)` 한 개로 확장됩니다.

`li`는 signed 12-bit 값이면 `addi` 한 개, 그 밖의 32-bit 값은 `lui`와 `addi` 두 개로 확장됩니다. `la`는 PC-relative `auipc`와 `addi` 두 개로 확장되며 Pass 1에서 8바이트를 예약합니다.

구현에는 PDF 목록 외의 추가 pseudo alias도 포함되어 있습니다. 예를 들어 `tail`, `seqz`, `snez`, `bltz`, `bgez`, `blez`, `bgtz`, `sltz`, `sgtz`, `csrs`, `csrc`, `csrwi`를 처리합니다. 이 목록은 과제 필수 범위를 넓히지 않으며 CPUlator에서 실행 검증한 기능을 뜻하지 않습니다.

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
mkdir -p output

for file in examples/*.s; do
    name=$(basename "$file" .s)

    ./assembler "$file" \
        "output/${name}.bin" \
        "output/${name}.lst" || exit 1
done
```

이 명령은 현재 예제 파일을 전부 시도합니다. 최신 최종 재검증에서는 예제 9개 모두 어셈블 성공했습니다. 교수님 제공 예제 1~4의 상세 결과는 [검증 결과](docs/RESULTS.md)에 기록했습니다.

## 테스트

현재 `tests/`에는 C 테스트 15개가 있습니다(기존 13개와 immediate/system 범위 추가 테스트 2개). 다음 명령은 파일 테스트에 필요한 예제 인자를 전달합니다.

```bash
test_dir=$(mktemp -d)
trap 'rm -rf "$test_dir"' EXIT

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
```

검증 기록에는 이 C 테스트 15개, assembly fixture 13개, 교수님 예제 4개 재어셈블 결과가 포함되어 있습니다. `docs/RESULTS.md`에 검증 범위와 한계를 정리했습니다.

## CPUlator에서 실행하는 방법

대상 환경은 CPUlator RISC-V DE10-Lite(`rv32-de10-lite`)입니다.

1. 어셈블러로 `.bin` 파일을 생성합니다.
2. CPUlator에서 **File → Load/Save Memory**를 엽니다.
3. Format을 `Raw`, Element size를 `1 byte`로 설정합니다.
4. Start address에 `10000`(16진수 `0x00010000`)을 입력하고 바이너리를 로드합니다.
5. Disassembly에서 주소 `00010000`의 첫 명령어를 확인합니다.
6. PC를 `00010000`으로 설정한 뒤 실행합니다.

Raw Binary에는 시작 주소 메타데이터가 없으므로 메모리 로드 주소와 PC 설정은 별도로 진행해야 합니다. 이 안내는 실행 절차이며, 이 문서의 최신 코드 검증에서 CPUlator 실행을 완료했다는 의미는 아닙니다.

## 문서

- [구현 과정 및 동작 상세](docs/PROCESS.md)
- [검증 과정 및 결과](docs/RESULTS.md)
- [AI 사용 및 프롬프트 기록](docs/AI_PROMPTS.md)

## 제한 사항 및 AI 사용 고지

- ELF 생성, 링커, relocation 및 여러 오브젝트 파일 링크는 지원하지 않습니다.
- BSS는 Raw Binary에 포함하지 않습니다.
- 일부 인코딩 오류는 mnemonic 중심으로 출력하며 모든 operand에 대한 상세 진단을 제공하지는 않습니다.
- `sret`, `fence.i`, `uret`, F/D, A, C 확장 및 링커/ELF 기능은 지원하지 않습니다.
- 최신 재검증에서 CPUlator 실행은 수행하지 않았습니다. 실제 보드 실행도 검증하지 않았습니다.

본 프로젝트의 설계, 코드 작성·수정, 오류 분석 및 테스트 과정에서 생성형 AI를 사용했습니다. 대표적인 프롬프트와 AI 및 사용자 역할은 [AI 사용 및 프롬프트 기록](docs/AI_PROMPTS.md)에 정리했습니다.
