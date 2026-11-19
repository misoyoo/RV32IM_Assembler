# RV32IM Assembler

C로 구현한 2-pass RISC-V RV32IM 어셈블러입니다. 어셈블리 소스 파일을 읽어 raw binary 파일(`.bin`)과 어셈블리 결과를 확인할 수 있는 listing 파일(`.lst`)을 생성합니다.

## 주요 기능

- **2-pass 구조**
  - Pass 1: 섹션 크기를 계산하고 라벨 및 심볼 정보를 등록합니다.
  - Pass 2: 명령어를 기계어로 인코딩하고 초기화 데이터를 바이너리에 기록합니다.
- **명령어 인코딩**: R, I, S, B, U, J 형식과 CSR 및 System 명령어 인코딩을 지원합니다.
- **레지스터 이름**: `x0`~`x31` 및 ABI 레지스터 이름을 지원합니다. `fp`는 `s0`의 별칭으로 처리합니다.
- **심볼 처리**: 라벨, 전역 심볼(`.global`, `.globl`), 상수 심볼(`.equ`)을 처리합니다.
- **의사 명령어**: 구현된 의사 명령어를 Pass 2에서 실제 명령어로 확장합니다.
- **데이터 및 정렬**: `.text`, `.data`, `.bss` 섹션과 데이터 지시어, 정렬을 처리합니다.
- **Listing 생성**: 주소, 기계어, 원본 소스 줄 및 심볼 정보를 `.lst` 파일에 기록합니다.
- **오류 검사**: 잘못된 명령어, 중복 라벨, 잘못된 레지스터, 인코딩할 수 없는 즉시값 및 지원하지 않는 BSS 지시어 등을 검사합니다.

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

정상적으로 처리되면 Pass 1과 Pass 2를 수행하고 바이너리 크기를 검증한 뒤 `ASSEMBLY SUCCESS!`를 출력합니다.

## 메모리 배치

- 기본 `.text` 시작 주소는 `0x00010000`입니다.
- `.data` 시작 주소는 `.text` 끝 주소를 기준으로 16바이트 경계에 맞춰 계산합니다.
- `.bss`는 `.data` 영역 뒤에 배치합니다. BSS 내부의 `.space` 및 `.align`으로 공간과 정렬을 계산합니다.
- raw binary에는 `.text`와 `.data`가 포함됩니다. `.bss`는 바이너리에 포함하지 않으며 BSS 라벨은 심볼 정보에서 확인할 수 있습니다.

## 지원 지시어

| 지시어 | 설명 |
|---|---|
| `.text` | 코드 영역 선택 |
| `.data` | 초기화 데이터 영역 선택 |
| `.bss` | 초기화되지 않은 데이터 영역 선택 |
| `.byte` | 1바이트 데이터 기록 |
| `.half` | 2바이트 데이터 기록 |
| `.word` | 4바이트 데이터 기록 |
| `.ascii` | 문자열 기록 |
| `.asciz` | 널 종료 문자를 포함한 문자열 기록 |
| `.space` | 지정한 크기만큼 공간 확보 |
| `.align` | 지정한 거듭제곱 경계로 정렬 |
| `.equ` | 상수 심볼 정의 |
| `.global`, `.globl` | 전역 심볼 선언 |

`.align N`은 `2^N` 바이트 경계로 정렬합니다. 현재 `.bss`에서는 `.space`와 `.align`만 지원하며, 그 외 지시어는 오류로 처리합니다.

`.equ` 값에는 숫자 및 문자 리터럴, 심볼 참조, 괄호, 단항 연산, 산술 연산, 시프트 연산 및 비트 연산을 사용하는 표현식을 사용할 수 있습니다.

## 예제 및 검증 문서

`examples/`에는 `asm_example1.s`~`asm_example4.s`, `ex01_getting_started.s`, `interval_timer.s`, `jtag_uart_echo.s`, `seven_segment_display.s`, `bss_layout_test.s`가 포함됩니다.

- [구현 과정 및 동작 상세](docs/PROCESS.md)
- [검증 과정 및 결과](docs/RESULTS.md)
- [AI 사용 및 프롬프트 기록](docs/AI_PROMPTS.md)

예제를 한 번에 실행하려면 다음 명령어를 사용할 수 있습니다.

```bash
mkdir -p output
for file in examples/*.s; do
    name=$(basename "$file" .s)
    ./assembler "$file" "output/${name}.bin" "output/${name}.lst" || break
done
```

## 출력 파일과 제한 사항

- **`.bin`**: `.text`와 `.data`가 배치된 raw binary 파일입니다. ELF 파일은 아닙니다.
- **`.lst`**: 각 출력 항목의 주소, 기계어, 대응 소스 줄 및 심볼 테이블을 확인할 수 있는 listing 파일입니다.
- 링커 기능, ELF 생성 및 외부 오브젝트 파일 링크는 구현 범위에 포함되지 않습니다.
- `.bss`는 바이너리 크기에 포함하지 않습니다.
