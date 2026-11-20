# 구현 과정 및 동작 상세

## 1. 프로젝트 개요

본 프로젝트는 C 언어로 구현한 RISC-V RV32IM 기반 2-pass 어셈블러이다.

어셈블리 소스 파일을 입력받아 명령어와 데이터의 메모리 배치를 계산하고, 기계어로 인코딩하여 Raw Binary(`.bin`)와 Listing(`.lst`) 파일을 생성한다.

어셈블러는 Pass 1과 Pass 2로 구성된다.

1. Parser를 이용해 소스 코드를 라벨, mnemonic, operand로 분리한다.
2. Pass 1에서 명령어 및 데이터 크기를 계산하고 메모리 주소와 심볼 정보를 등록한다.
3. Pass 2에서 명령어를 기계어로 인코딩하고 초기화 데이터를 바이트 배열에 기록한다.
4. 바이너리 생성 크기를 검증하고 `.bin` 및 `.lst` 파일을 출력한다.

본 프로젝트는 ELF 파일 생성이나 링커 기능은 포함하지 않는다. 출력 파일은 little-endian Raw Binary 형식이다.

## 2. 소스 파일별 역할

| 파일 | 역할 |
|---|---|
| `main.c` | 명령행 인자 처리, 파일 입출력, Pass 1·2 실행, 결과 크기 검증 |
| `parser.c`, `parser.h` | 어셈블리 소스 줄을 파싱 |
| `register.c`, `register.h` | 레지스터 이름을 번호로 변환 |
| `encoder.c`, `encoder.h` | 명령어 형식별 기계어 인코딩 |
| `directives.c`, `directives.h` | 지시어, 표현식, 문자열 및 escape 처리 |
| `symbol_table.c`, `symbol_table.h` | 심볼 주소, 유형, 전역 여부 및 정의 여부 관리 |
| `pass1.c`, `pass1.h` | 섹션 크기 계산, 주소 배치 및 심볼 등록 |
| `pass2.c`, `pass2.h` | 기계어·데이터 생성, Listing 및 Binary 출력 |

테스트용 C 프로그램은 `tests/`, 어셈블리 검증 예제는 `examples/`에 저장한다.

## 3. Parser 및 Register

Parser는 한 줄의 어셈블리 코드를 `ParsedLine` 구조체로 변환한다.

주요 필드는 다음과 같다.

- `label`: 라벨 이름
- `mnemonic`: 명령어 또는 지시어 이름
- `operand_count`: operand 개수
- `operands`: operand 문자열 배열

빈 줄, 주석 줄, 라벨만 존재하는 줄을 처리하며 메모리 주소 참조 형태의 operand와 문자열 내부 쉼표도 고려한다.

Register 모듈은 ABI 레지스터 이름과 `x0`~`x31` 형식을 내부 레지스터 번호로 변환한다. `fp`는 `s0`의 별칭으로 지원한다.

## 4. Encoder: 명령어 인코딩

Encoder는 명령어 형식에 맞게 각 비트 필드를 조합하여 32비트 기계어를 생성한다.

| 명령어 형식 | 주요 필드 |
|---|---|
| R-type | opcode, rd, funct3, rs1, rs2, funct7 |
| I-type | opcode, rd, funct3, rs1, immediate |
| S-type | opcode, funct3, rs1, rs2, immediate |
| B-type | opcode, funct3, rs1, rs2, branch offset |
| U-type | opcode, rd, upper immediate |
| J-type | opcode, rd, jump offset |
| CSR | CSR 주소 및 레지스터 또는 immediate 필드 |
| System | 시스템 명령어에 해당하는 고정 및 지정 필드 |

라벨을 사용하는 분기 및 점프 명령어는 심볼 테이블의 주소와 현재 명령어 주소의 차이를 이용하여 offset을 계산한다.

구현된 의사 명령어는 실제 명령어 인코딩으로 확장한다. `jal`은 목적 레지스터를 생략한 형태에서 기본 목적 레지스터로 `ra(x1)`를 사용하는 동작을 지원한다.

## 5. 지시어 처리

다음 지시어를 지원한다.

- 섹션: `.text`, `.data`, `.bss`
- 심볼: `.equ`, `.global`, `.globl`
- 데이터: `.byte`, `.half`, `.word`, `.ascii`, `.asciz`
- 공간 및 정렬: `.space`, `.align`

표현식 평가 기능은 숫자 및 문자 리터럴, 심볼 참조, 괄호, 단항 연산, 산술 연산, 시프트 연산 및 비트 연산을 처리한다.

문자열 지시어에서는 escape sequence를 해석한다.

`.align N`은 다음 `2^N` 바이트 경계로 정렬한다. `.bss`에서는 초기화 데이터 지시어 대신 `.space`와 `.align`을 사용한다.

## 6. Symbol Table

심볼 테이블은 동적 배열을 사용하며 `realloc()`을 통해 저장 공간을 확장한다.

각 심볼은 이름, 주소, 유형, 전역 여부 및 정의 여부를 저장한다.

| 유형 | 의미 |
|---|---|
| `T` | Text 영역 심볼 |
| `D` | Data 영역 심볼 |
| `B` | BSS 영역 심볼 |
| `A` | 절대값 심볼 |
| `U` | 선언되었지만 아직 정의되지 않은 전역 심볼 |

`.global` 및 `.globl` 선언은 실제 심볼 정의와 연결한다. 정의되지 않은 심볼이 명령어 인코딩 과정에서 참조되면 오류로 처리한다.

## 7. Pass 1: 메모리 배치 및 심볼 주소 계산

기본 Text 시작 주소는 `0x00010000`이다.

Pass 1은 소스 파일을 여러 차례 스캔하여 다음 정보를 계산한다.

1. Text 영역의 명령어 크기
2. Data 영역의 시작 주소 및 크기
3. BSS 영역의 시작 주소 및 크기
4. 각 라벨의 메모리 주소
5. `.equ` 절대값 심볼
6. 전역 심볼 선언 및 정의 여부

Data 시작 주소는 Text 영역의 끝 주소를 16바이트 경계로 정렬하여 계산한다.

```text
data_base = align_up(text_base + text_size, 16)
```

BSS 시작 주소는 Data 영역 바로 뒤에서 시작한다.

```text
bss_base = data_base + data_size
```

BSS 시작 주소 자체를 별도로 16바이트 정렬하지는 않는다. BSS 내부 정렬이 필요한 경우 `.align`을 사용한다.

현재 BSS에서 지원하는 크기 지시어는 `.space`와 `.align`이다.

## 8. Pass 2: 기계어 생성 및 데이터 출력

Pass 2는 Pass 1에서 생성한 섹션 레이아웃과 심볼 테이블을 이용한다.

- 명령어를 형식별 Encoder로 전달한다.
- 라벨을 참조하는 분기 및 점프 offset을 계산한다.
- 의사 명령어를 구현된 확장 규칙에 따라 실제 명령어로 출력한다.
- `.byte`, `.half`, `.word` 값을 little-endian 바이트 배열에 기록한다.
- `.ascii`, `.asciz` 문자열과 종료 문자를 출력한다.
- `.space`, `.align`에 필요한 공간 및 정렬을 반영한다.
- BSS는 주소와 크기만 관리하고 Raw Binary에는 초기화 바이트를 포함하지 않는다.

최종 바이너리에는 Text 영역과 Data 영역 사이의 정렬 간격 및 초기화 데이터가 포함될 수 있다. BSS는 바이너리 크기에 포함하지 않는다.

## 9. Listing 파일 생성

Listing 파일에는 출력 항목의 주소, 기계어 또는 데이터 값, 원본 소스 줄을 기록한다.

마지막에는 심볼 테이블을 출력하여 각 심볼의 주소, 유형 및 전역 여부를 확인할 수 있도록 한다.

따라서 Listing 파일을 이용하면 실제 명령어가 `0x00010000`부터 배치되었는지 확인할 수 있다.

## 10. 프로그램 시작 주소 및 CPUlator 실행

기본 Text 시작 주소는 `pass1.h`에 정의되어 있다.

```c
#define DEFAULT_TEXT_BASE 0x00010000u
```

출력 파일은 Raw Binary이므로 실행 시작 주소에 관한 ELF 헤더나 별도의 주소 메타데이터가 없다.

CPUlator의 RISC-V DE10-Lite 환경에서 실행할 때는 다음 절차를 따른다.

1. Load/Save Memory에서 Format을 `Raw`로 지정한다.
2. Element size를 `1 byte`로 지정한다.
3. Start address를 `10000`으로 입력한다.
4. 생성된 `.bin` 파일을 메모리에 로드한다.
5. Disassembly에서 첫 명령어 주소인 `00010000`으로 이동한다.
6. PC를 `00010000`으로 설정한 뒤 실행한다.

메모리 로드 주소와 PC는 서로 다른 개념이다. Raw Binary를 메모리에 로드하는 것만으로 PC가 자동 변경되지는 않는다.

PC를 `0x00010000`으로 설정하는 것은 프로그램의 실제 시작 위치를 지정하는 실행 환경 설정이다.

## 11. 오류 처리

다음 오류를 검출하도록 구현했다.

- 알 수 없는 mnemonic
- 중복 또는 유효하지 않은 라벨
- 정의되지 않은 심볼 참조
- 잘못된 레지스터 이름
- 인코딩 범위를 벗어난 immediate
- BSS에서 지원하지 않는 지시어

Pass 1에서 검출할 수 있는 명령어 및 섹션 오류는 Pass 1에서 처리하고, 레지스터 및 immediate 등 인코딩 과정에서 확인되는 오류는 Pass 2에서 처리한다.

다만 모든 오류에 대해 잘못된 operand의 종류와 위치까지 상세히 출력하는 진단 기능이 완성된 것은 아니다. 일부 인코딩 오류는 `cannot encode '<mnemonic>'` 형식으로 출력된다.

## 12. 구현 범위 및 제한사항

- 출력은 little-endian Raw Binary와 Listing이다.
- ELF 생성, 링킹 및 relocation 기능은 포함하지 않는다.
- BSS는 주소와 크기 및 심볼 정보로 관리하며 초기화 바이너리 바이트를 생성하지 않는다.
- BSS 시작 주소 자체를 자동으로 별도 정렬하지 않는다.
- BSS에서는 `.space`, `.align`만 지원한다.
- 모든 인코딩 오류에 대해 operand별 상세 진단 메시지를 출력하지는 않는다.