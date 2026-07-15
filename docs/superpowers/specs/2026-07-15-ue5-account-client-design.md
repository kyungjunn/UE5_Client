# UE5 계정 서버 통신 클라이언트 — 설계 문서

작성일: 2026-07-15

## 개요

UE5 클라이언트(`UEClient_20260715`)에서 계정 백엔드 서버(FastAPI + MySQL)와
REST(JSON)로 통신하는 C++ 클라이언트를 구현한다. 회원가입 · 로그인 · 내 정보 조회 ·
회원탈퇴 4개 기능을 제공한다.

현재 프로젝트는 블루프린트 전용(Source 없음)이므로, C++ 모듈을 새로 추가해
C++ 프로젝트로 전환한다.

- 엔진: UE 5.7 (`C:/Program Files/Epic Games/UE_5.7`)
- 서버 기본 주소: `http://127.0.0.1:8000`

## 서버 계약 (확인됨)

서버 소스(`C:/work/VibeCoding/app`)에서 직접 확인한 실제 계약.

| API | 요청 본문 | 성공 응답 | 실패 |
|-----|-----------|-----------|------|
| POST `/auth/register` | `{email, nickname(2~50), password(8~128)}` | 201 `{id, email, nickname, created_at}` | 409 / 422 `{detail}` |
| POST `/auth/login` | `{email, password}` | 200 `{access_token, token_type}` | 401 `{detail}` |
| GET `/auth/me` | (Bearer) | 200 `{id, email, nickname, created_at}` | 401 |
| DELETE `/auth/withdraw` | (Bearer) | 204 (본문 없음) | 401 |

- 인증: `Authorization: Bearer <access_token>` (JWT, HS256)
- 에러 본문: FastAPI 표준. HTTPException은 `{"detail": "문자열"}`,
  검증 실패(422)는 `{"detail": [{"msg": "...", ...}]}` (배열)

## 아키텍처 (A안)

핵심 클래스 하나(`UAccountSubsystem`)가 토큰과 4개 API 호출을 모두 관리한다.
`UGameInstanceSubsystem`이라 인스턴스가 하나이고 레벨 전환에도 유지되며,
블루프린트에서 접근 가능하다.

```
Source/UEClient_20260715/
  UEClient_20260715.Build.cs      # 모듈 의존성: Core, CoreUObject, Engine, HTTP, Json, JsonUtilities
  UEClient_20260715.h / .cpp      # IMPLEMENT_PRIMARY_GAME_MODULE
  AccountSubsystem.h / .cpp       # 통신 클라이언트 (핵심)
Source/UEClient_20260715.Target.cs
Source/UEClient_20260715Editor.Target.cs
UEClient_20260715.uproject        # "Modules" 항목 추가
```

### 데이터 구조

```cpp
USTRUCT(BlueprintType)
struct FAccountUser
{
    int32   Id;
    FString Email;
    FString Nickname;
    FString CreatedAt;
};
```

### 델리게이트 (다이내믹 멀티캐스트 — BP 바인딩용) — 2종

```cpp
// Login, Withdraw
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FAccountSimpleResult, bool, bSuccess, int32, StatusCode, const FString&, Message);

// Register, Me
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
    FAccountUserResult, bool, bSuccess, int32, StatusCode, const FString&, Message,
    const FAccountUser&, User);
```

### UAccountSubsystem

상태:
- `FString BaseUrl` — 기본 `http://127.0.0.1:8000`
- `FString AuthToken` — 로그인 성공 시 저장, 메모리에만 보관 (재시작 시 소멸)

`UFUNCTION(BlueprintCallable)` 메서드:
- `Register(Email, Nickname, Password)` → 완료 시 `OnRegisterCompleted` 브로드캐스트
- `Login(Email, Password)` → 성공 시 토큰 저장, `OnLoginCompleted` 브로드캐스트
- `GetMe()` → Bearer 사용, `OnMeCompleted` 브로드캐스트
- `Withdraw()` → Bearer 사용, 성공 시 토큰 삭제, `OnWithdrawCompleted` 브로드캐스트
- `SetBaseUrl(Url)` → 서버 주소 변경

완료 델리게이트(BlueprintAssignable) 4개:
`OnRegisterCompleted`, `OnLoginCompleted`, `OnMeCompleted`, `OnWithdrawCompleted`

### 콘솔 명령어

`Initialize()`에서 `IConsoleManager`로 등록 (서브시스템 소멸 시 해제):
- `Account.Register <email> <nickname> <password>`
- `Account.Login <email> <password>`
- `Account.Me`
- `Account.Withdraw`
- `Account.SetBaseUrl <url>`

각 명령은 대응 메서드를 호출하고, 결과 델리게이트를 구독해 `UE_LOG`로 출력한다.

## 데이터 흐름

1. 호출부(BP 또는 콘솔) → 서브시스템 메서드
2. 메서드가 `FHttpRequest` 구성 (URL, 메서드, 헤더, JSON 본문)
   - 인증 필요 API는 `Authorization: Bearer <AuthToken>` 헤더 추가
3. 비동기 전송, 완료 콜백(`OnProcessRequestComplete`)에서 응답 파싱
4. 결과를 델리게이트로 브로드캐스트

## 에러 처리

- 연결 실패(`bConnectedSuccessfully == false` 또는 응답 없음)
  → `bSuccess=false, StatusCode=0, Message="서버에 연결할 수 없습니다."`
- 2xx → `bSuccess=true`
- 비2xx → `bSuccess=false`, 본문의 `detail` 파싱
  - `detail`이 문자열이면 그대로 Message
  - `detail`이 배열(422)이면 첫 항목의 `msg`를 Message
  - 파싱 실패 시 상태 코드 기반 기본 메시지
- 인증 필요 API인데 `AuthToken`이 비어 있으면
  → 요청을 보내지 않고 즉시 `bSuccess=false, StatusCode=0, Message="로그인이 필요합니다."`
- 204(withdraw)는 본문이 없으므로 파싱하지 않고 성공 처리

## 검증 방법

1. **컴파일**: UBT로 프로젝트 파일 재생성 후 모듈 빌드 성공.
2. **엔드투엔드(수동)**: 서버(uvicorn + MySQL) 기동 상태에서 콘솔로 순차 실행
   - `Account.Register a@b.com nick password123` → 성공 로그, 재실행 시 409
   - `Account.Login a@b.com password123` → 토큰 저장 로그
   - `Account.Me` → 내 정보(id/email/nickname) 로그
   - `Account.Withdraw` → 성공(204) 로그, 이후 `Account.Login` 401
   - 서버측 pytest 시나리오와 동일한 결과를 기대.

## 전제 조건

- Visual Studio 2022 (MSVC + "C++를 사용한 게임 개발" 워크로드) — 설치 확인됨.
- 엔드투엔드 검증 시 서버가 로컬에서 기동 중이어야 함.

## 범위 밖

- 로그인 UI/위젯 (나중에 BP에서 델리게이트 바인딩으로 연결)
- 토큰 디스크 영속화(자동 로그인), refresh 토큰
- 비밀번호 재설정, 소셜 로그인
