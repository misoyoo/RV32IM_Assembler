# 검증 과정 및 결과

## 1. 빌드 환경 및 명령

개발 및 테스트 환경은 macOS의 터미널과 C 컴파일러다. 프로젝트 루트에서 다음 명령으로 빌드했다.

```bash
gcc -Wall -Wextra -std=c11 -o assembler \
    main.c pass2.c pass1.c parser.c directives.c \
    symbol_table.c encoder.c register.c
```

## 2. 제공 예제 어셈블 결과

다음 반복문으로 `examples/` 아래의 어셈블리 파일을 각각 `.bin`과 `.lst`로 생성했다.

```bash
mkdir -p output
for file in examples/*.s; do
    name=$(basename "$file" .s)
    ./assembler "$file" "output/${name}.bin" "output/${name}.lst" || break
done
```

마지막 전체 실행에서 아래 9개 예제가 모두 `ASSEMBLY SUCCESS!`로 종료됐고, 생성 바이너리 크기 검증도 통과했다.

| 예제 | Text 크기 | Data 크기 | 생성 Binary 크기 |
|---|---:|---:|---:|
| `asm_example1.s` | 680 B | 28 B | 716 B |
| `asm_example2.s` | 664 B | 124 B | 796 B |
| `asm_example3.s` | 800 B | 84 B | 884 B |
| `asm_example4.s` | 1,916 B | 460 B | 2,380 B |
| `ex01_getting_started.s` | 92 B | 0 B | 92 B |
| `interval_timer.s` | 76 B | 0 B | 76 B |
| `jtag_uart_echo.s` | 124 B | 27 B | 155 B |
| `seven_segment_display.s` | 348 B | 16 B | 368 B |
| `bss_layout_test.s` | 4 B | 8 B | 24 B |

Binary 크기는 text와 data 사이의 정렬 간격을 포함할 수 있다. 따라서 text 크기와 data 크기의 단순 합과 항상 같지는 않다. BSS 크기는 raw binary에 포함하지 않는다.

## 3. BSS 배치 검증

`bss_layout_test.s`에는 `.data`, `.align`, `.bss`, `.space`가 포함되어 있다. 확인된 결과는 다음과 같다.

| 항목 | 결과 |
|---|---:|
| Text 시작 주소 | `0x00010000` |
| Text 크기 | 4 B |
| Data 시작 주소 | `0x00010010` |
| Data 크기 | 8 B |
| BSS 시작 주소 | `0x00010018` |
| BSS 크기 | 1,096 B |
| `stack` 라벨 | `0x00010020` (`B`) |
| `tcb` 라벨 | `0x00010420` (`B`) |
| Binary 크기 | 24 B |

이 예제에서 BSS 시작 주소는 data 직후이며, `stack` 주소는 BSS 내부의 `.align 4` 적용 결과다. BSS 자체는 바이너리 바이트에 포함되지 않고 심볼 주소와 BSS 크기로 관리된다.

## 4. 오류 검증

아래 오류 사례를 별도 입력으로 실행해 Pass 1 또는 Pass 2가 실패하는지 확인했다.

| 테스트 | 예상/관찰된 처리 |
|---|---|
| 존재하지 않는 mnemonic `invalid_op` | Pass 1에서 실패: `unknown or invalid instruction 'invalid_op'` |
| `_start` 라벨 중복 정의 | Pass 1에서 실패: `duplicate or invalid label '_start'` |
| 정의되지 않은 `MISSING_LABEL`을 `jal`에서 참조 | Pass 2에서 실패: `cannot encode 'jal'` |
| 잘못된 레지스터 `badreg` 사용 | Pass 2에서 실패: `cannot encode 'addi'` |
| `addi` 즉시값 4096 사용 | Pass 2에서 실패: `cannot encode 'addi'` |
| BSS에서 `.word 123` 사용 | Pass 1에서 실패: `only .space and .align are supported in .bss` |

추가로 잘못된 즉시값과 잘못된 BSS 지시어 테스트를 다시 실행하기 전에 이전 결과 파일을 제거한 후, 실패 실행에서 해당 `.bin` 및 `.lst` 출력 파일이 생성되지 않는 것을 확인했다.

## 5. 검증 범위와 한계

- 제공된 8개 원본 예제와 BSS 배치 검증 예제 1개를 어셈블했다.
- Pass 1에서 섹션 크기와 심볼을 계산하고 Pass 2에서 최종 바이너리와 listing을 생성하는 흐름을 확인했다.
- 대표적인 문법·심볼·레지스터·즉시값·BSS 오류를 확인했다.
- 이 검증은 어셈블 및 파일 생성 확인이다. 생성한 바이너리를 실제 DE10-Lite/CPU에 로드해 실행한 검증은 아직 수행하지 않았다.
- 따라서 과제의 동작 동영상 및 실제 타깃 실행 확인은 별도 단계로 남아 있다.
