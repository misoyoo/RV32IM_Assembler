# RV32IM Assembler

C로 구현한 2-pass RISC-V RV32IM 어셈블러입니다. 어셈블리 소스 파일을 읽어 Raw Binary 파일(`.bin`)과 어셈블 결과를 확인할 수 있는 Listing 파일(`.lst`)을 생성합니다.

이 프로젝트의 출력 파일은 ELF가 아닌 little-endian Raw Binary입니다. 따라서 바이너리 파일 자체에는 실행 시작 주소나 relocation 정보가 포함되지 않습니다.

## 주요 기능

- **2-pass 구조**
  - Pass 1: 섹션 크기를 계산하고 메모리 주소 및 라벨·심볼 정보를 등록합니다.
  - Pass 2: 명령어를 기계어로 인코딩하고 초기화 데이터를 바이너리에 기록합니다.
- **명령어 인코딩**: R, I, S, B, U, J 형식과 CSR 및 System 명령어 인코딩을 지원합니다.
- **레지스터 이름**: `x0`~`x31` 및 ABI 레지스터 이름을 지원합니다. `fp`는 `s0`의 별칭으로 처리합니다.
- **심볼 처리**: 라벨, 전역 심볼(`.global`, `.globl`), 상수 심볼(`.equ`)을 처리합니다.
- **의사 명령어**: 구현된 의사 명령어를 Pass 2에서 실제 명령어로 확장합니다.
- **데이터 및 정렬**: `.text`, `.data`, `.bss` 섹션과 데이터 지시어 및 정렬을 처리합니다.
- **Listing 생성**: 주소, 기계어 또는 데이터 값, 원본 소스 줄 및 심볼 정보를 `.lst` 파일에 기록합니다.
- **오류 검사**: 잘못된 명령어, 중복 라벨, 정의되지 않은 심볼, 잘못된 레지스터, 인코딩할 수 없는 즉시값 및 지원하지 않는 BSS 지시어 등을 검사합니다.
- **바이너리 크기 검증**: 생성된 바이너리 크기와 Pass 1에서 계산한 예상 크기를 비교합니다.

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
├── examples/                 # 어셈블리 입력 예제 및 검증용 소스
├── tests/                    # 모듈별 C 테스트 프로그램
├── docs/
│   ├── PROCESS.md            # 구현 과정 및 어셈블러 동작 상세
│   ├── RESULTS.md            # 검증 과정 및 결과
│   └── AI_PROMPTS.md         # AI 사용 및 프롬프트 기록
├── output/                   # 생성되는 .bin 및 .lst 파일
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

정상적으로 처리되면 Pass 1과 Pass 2를 수행하고 바이너리 크기를 검증한 뒤 `ASSEMBLY SUCCESS!`를 출력합니다.

## 메모리 배치

- 기본 `.text` 시작 주소는 `0x00010000`입니다.
- `.data` 시작 주소는 `.text` 끝 주소를 기준으로 16바이트 경계에 맞춰 계산합니다.
- `.bss`는 `.data` 영역 뒤에 배치합니다. BSS 시작 주소 자체를 별도로 16바이트 정렬하지는 않습니다.
- BSS 내부의 `.space` 및 `.align`으로 공간과 정렬을 계산합니다.
- Raw Binary에는 `.text`와 `.data`가 포함됩니다. `.bss`는 바이너리에 포함하지 않으며 BSS 라벨은 심볼 정보에서 확인할 수 있습니다.

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

## 예제 및 일괄 검증

`examples/`에는 다음 어셈블리 예제가 포함됩니다.

- `asm_example1.s` ~ `asm_example4.s`: 주요 과제 예제
- `ex01_getting_started.s`
- `interval_timer.s`
- `jtag_uart_echo.s`
- `seven_segment_display.s`
- `bss_layout_test.s`: BSS 배치 검증용 예제

모든 예제를 한 번에 어셈블하려면 다음 명령을 사용할 수 있습니다.

```bash
mkdir -p output
for file in examples/*.s; do
    name=$(basename "$file" .s)
    ./assembler "$file" "output/${name}.bin" "output/${name}.lst" || break
done
```

C 기반 모듈 테스트 프로그램은 `tests/`에 정리되어 있습니다.

## CPUlator에서 실행

주요 예제의 실행 검증에는 CPUlator의 RISC-V DE10-Lite 시뮬레이터를 사용합니다.

1. 어셈블러를 실행해 해당 예제의 `.bin` 파일을 생성합니다.
2. CPUlator에서 **File → Load/Save Memory**를 엽니다.
3. Format을 `Raw`, Element size를 `1 byte`로 설정합니다.
4. Start address에 `10000`(16진수 `0x00010000`)을 입력하고 `.bin` 파일을 로드합니다.
5. 로드 메시지에서 시작 주소가 `00010000`인지 확인합니다.
6. Disassembly에서 주소 `00010000`의 첫 명령어로 이동해 PC를 `00010000`으로 설정합니다.
7. Continue를 눌러 실행합니다.

**메모리 로드 주소와 PC는 서로 다른 설정입니다.** Raw Binary에는 시작 주소 메타데이터가 없으므로, 메모리의 로드 주소를 `0x00010000`으로 설정한 뒤 CPU의 PC도 `0x00010000`으로 설정해야 해당 위치에서 실행됩니다. PC를 시작 주소로 지정하는 것은 Raw Binary 실행 환경을 설정하는 절차이며, 바이너리 내용을 변경하는 것이 아닙니다.

## 검증 문서

프로젝트의 구현 과정, 검증 결과 및 AI 사용 기록은 다음 문서에서 확인할 수 있습니다.

- [구현 과정 및 동작 상세](docs/PROCESS.md): 모듈별 역할, Parser와 Encoder, Symbol Table, 지시어 처리, Pass 1·2 및 메모리 배치
- [검증 과정 및 결과](docs/RESULTS.md): 빌드 명령, 제공 예제 어셈블 결과, 시작 주소 및 BSS 검증, 오류 테스트, CPUlator 실행 검증
- [AI 사용 및 프롬프트 기록](docs/AI_PROMPTS.md): AI 사용 범위, 초기 생성 프롬프트, 기능 구현·검증·디버깅 프롬프트

## 출력 파일과 제한 사항

- **`.bin`**: `.text`와 `.data`가 배치된 little-endian Raw Binary 파일입니다. ELF 파일은 아닙니다.
- **`.lst`**: 각 출력 항목의 주소, 기계어 또는 데이터 값, 대응 소스 줄 및 심볼 테이블을 확인할 수 있는 Listing 파일입니다.
- 링커 기능, ELF 생성 및 외부 오브젝트 파일 링크는 구현 범위에 포함되지 않습니다.
- `.bss`는 바이너리 크기에 포함하지 않고 주소와 크기 및 심볼 정보로 관리합니다.
- 인코딩 오류는 검출하지만, 모든 오류에 대해 잘못된 operand의 종류와 위치까지 상세히 설명하는 진단 메시지를 제공하지는 않습니다.

## AI 사용 고지

본 프로젝트의 설계, 코드 작성·수정, 오류 분석 및 테스트 절차 구성에 생성형 AI를 사용했습니다. 대표적인 초기 생성·검증 프롬프트와 AI 및 사용자 역할 구분은 [AI 사용 및 프롬프트 기록](docs/AI_PROMPTS.md)에 정리했습니다.
