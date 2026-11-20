## Documentation

프로젝트의 구현 과정, 검증 결과 및 AI 사용 기록은 다음 문서에서 확인할 수 있습니다.

| 문서 | 내용 |
|---|---|
| [`docs/PROCESS.md`](docs/PROCESS.md) | 어셈블러 전체 구조, Parser, Encoder, Symbol Table, Pass 1·2, 메모리 배치 및 Binary·Listing 생성 방식 |
| [`docs/RESULTS.md`](docs/RESULTS.md) | 빌드 방법, 제공 예제 9개의 어셈블 결과, `0x00010000` 시작 주소 검증, BSS 테스트, 오류 검증 및 CPUlator 실행 결과 |
| [`docs/AI_PROMPTS.md`](docs/AI_PROMPTS.md) | 생성형 AI 사용 여부, 초기 생성 프롬프트, 기능 구현 프롬프트, 검증 및 디버깅 프롬프트 |

### Verification Summary

- 9개 어셈블리 예제의 Pass 1·Pass 2 및 Binary Validation 성공
- 주요 예제 1~4의 Text Base 및 Listing 첫 명령어 주소 `0x00010000` 확인
- BSS 레이아웃 및 오류 입력 테스트
- CPUlator RISC-V DE10-Lite에서 주요 예제 실행 및 기능 확인

AI 사용 사실과 대표 프롬프트는 `docs/AI_PROMPTS.md`에 기록했습니다.
