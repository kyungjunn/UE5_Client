# UE5_Client

언리얼 엔진 5.7 클라이언트. **계정 백엔드 서버(FastAPI + MySQL)** 와 REST로 통신하는
계정 시스템과, **OnlineSubsystem(NULL/LAN) 세션 기반 로비/방 시스템**을 C++로 구현했다.

기존 블루프린트 전용 프로젝트를 C++ 프로젝트로 전환해 진행 중이다.

---

## 주요 기능

### 1. 계정 (서버 통신)
회원가입 · 로그인 · 내 정보 조회 · 회원탈퇴. 로그인 시 받은 JWT를 메모리에 보관한다.

### 2. 로그인 / 회원가입 UI
`UAccountSubsystem`을 호출하고 결과를 UI에 반영하는 UserWidget C++ 베이스. 화면 전환(Signup ↔ Login) 포함.

### 3. 로비 / 세션(방) 시스템
로그인 성공 → Lobby로 이동해 닉네임 표시 → 방 생성 / 방 목록 조회 / 방 참가.
방 안에서는 접속한 모든 플레이어의 닉네임을 서로 확인한다. (최대 4인, LAN)

---

## 아키텍처

| 항목 | 결정 |
|------|------|
| 통신 방식 | C++ (언리얼 HTTP 모듈), REST/JSON |
| 계정 아키텍처 | `UGameInstanceSubsystem` 단일 클래스 |
| 호출 방식 | `BlueprintCallable` 함수 + 콘솔 명령어 |
| 토큰 보관 | JWT 메모리 보관 (재시작 시 소멸) |
| OnlineSubsystem | NULL (LAN) — 리슨 서버 모델 |
| 레벨 구조 | Login → Lobby(브라우저) → Room(실제 방) 분리 |
| 닉네임 보관 | 커스텀 `UGameInstance`에 캐시 |

---

## 서버 계약

기본 주소 `http://127.0.0.1:8000` (`SetBaseUrl`로 변경). 인증: `Authorization: Bearer <access_token>` (JWT, HS256).

| API | 요청 본문 | 성공 응답 | 실패 |
|-----|-----------|-----------|------|
| POST `/auth/register` | email, nickname(2~50), password(8~128) | 201 (id, email, nickname, created_at) | 409 / 422 `detail` |
| POST `/auth/login` | email, password | 200 (access_token, token_type) | 401 `detail` |
| GET `/auth/me` | (Bearer) | 200 (id, email, nickname, created_at) | 401 |
| DELETE `/auth/withdraw` | (Bearer) | 204 | 401 |

에러 본문: FastAPI 표준 `{"detail": "..."}` (문자열) / 422는 `{"detail": [{"msg": ...}]}` (배열).

---

## 레벨 흐름

```
[Login] --로그인 성공(GetMe)--> [Lobby (standalone): 프로필 + 방만들기 + RoomList]
  방 만들기 → CreateSession(max 4) → ServerTravel("Room?listen")
  방 참가   → JoinSession → ClientTravel → [Room (listen server)]
[Room] LobbyGameMode/PC/PlayerState → 접속자 닉네임 목록 / 나가기 → Lobby 복귀
```

---

## 프로젝트 구조

```
Source/UEClient_20260715/
  UEClient_20260715.Build.cs        # 의존성: Core, Engine, UMG, HTTP, Json, Slate, OnlineSubsystem(+Utils)
  AccountSubsystem.h/.cpp           # 계정 서버 통신 클라이언트 (핵심)
  AuthUserWidgetBase.h/.cpp         # 로그인/가입 위젯 공통 베이스
  LoginUserWidget.h/.cpp            # 로그인 위젯 (성공 시 GetMe → 닉네임 캐시 → Lobby 이동)
  RegisterUserWidget.h/.cpp         # 회원가입 위젯
  UEClientGameInstance.h/.cpp       # 로그인 닉네임 캐시 (레벨 이동해도 유지)
  GameSessionSubsystem.h/.cpp       # 세션 래핑: Host/Find/Join/Leave, FRoomInfo
  LobbyGameMode.h/.cpp              # Room 맵 게임모드 (폰 없음, PC/PS 지정)
  LobbyPlayerController.h/.cpp      # 로컬 닉네임을 ServerSetNickname RPC로 서버 전달
  LobbyPlayerState.h/.cpp           # 복제되는 Nickname
  LobbyWidget.h/.cpp                # 프로필 + 방만들기 + RoomList(병합)
  RoomListEntryWidget.h/.cpp        # 방 한 줄 (꽉 차면 Join 비활성)
  RoomWidget.h/.cpp                 # 방 안 닉네임 목록 + 나가기
Config/DefaultEngine.ini            # GameInstanceClass, OnlineSubsystem=Null
docs/superpowers/specs/             # 설계 문서
작업내역.md / 작업내역.html         # 상세 작업 내역
```

---

## 빌드

Visual Studio 2022 + UE 5.7 설치 필요.

```bat
"C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" ^
  UEClient_20260715Editor Win64 Development ^
  -Project="<repo>\UEClient_20260715.uproject" -WaitMutex
```

> 신규 UClass·플러그인 추가 시 Live Coding으로 반영 불가 → **에디터 종료 후** 빌드.

---

## 사용법

### 콘솔 명령어 (PIE 콘솔, 백틱 `` ` `` 키)
```
Account.Register <email> <nickname> <password>
Account.Login <email> <password>
Account.Me
Account.Withdraw
Account.SetBaseUrl <url>
```
결과는 Output Log에 `[Account] ...`로 출력.

### 위젯 연동 (BindWidget 이름)

| WBP | 부모 클래스 | 배치할 위젯 (이름 정확히) |
|-----|-------------|---------------------------|
| `WBP_Login` | `LoginUserWidget` | `EmailInput`, `PasswordInput`, `LoginButton`, `SignupButton`, `StatusText`(선택) |
| `WBP_Register` | `RegisterUserWidget` | `EmailInput`, `NicknameInput`, `PasswordInput`, `RegisterButton`, `ExitButton`, `StatusText`(선택) |
| `WBP_Lobby` | `LobbyWidget` | `NicknameText`, `RoomNameInput`, `CreateRoomButton`, `RefreshButton`, `RoomListBox` |
| `WBP_RoomEntry` | `RoomListEntryWidget` | `RoomNameText`, `HostNameText`, `CountText`, `JoinButton` |
| `WBP_Room` | `RoomWidget` | `PlayerListBox`, `LeaveButton` |

- `WBP_Login` → Class Defaults → Account → `Register Widget Class` = `WBP_Register`, `Lobby Level Name` = `Lobby`
- `WBP_Lobby` → Class Defaults → Session → `Room Entry Widget Class` = `WBP_RoomEntry`
- `Room` 맵 → World Settings → `GameMode Override` = `LobbyGameMode`
- 각 맵 Level Blueprint(BeginPlay)에서 해당 위젯 Create + Add to Viewport

---

## 테스트

### 계정 흐름
1. 서버 기동 (MySQL `game_account` DB 켜진 상태):
   ```
   uvicorn app.main:app --reload
   ```
2. PIE 콘솔에서: 가입 201 → 중복 409 → 로그인 → 내 정보 → 탈퇴 204 → 재로그인 401.

### 로비 / 세션
1. Editor Play 모드 = **Standalone**, Number of Players = **2**
   (PIE 다중창은 GameInstance 공유로 닉네임 이슈 → Standalone 권장)
2. 각 인스턴스에서 서로 다른 계정 로그인 → 자동 Lobby 진입, 닉네임 표시
3. 한쪽 방 생성 → 다른쪽 새로고침 → RoomList에서 참가 → Room에서 서로 닉네임 확인
4. 4/4인 방은 Join 비활성 / 새로고침 시 목록이 갱신되되 기존 항목은 사라지지 않음

---

## 상태

- ✅ 컴파일·링크 성공 (계정 / 로그인·가입 위젯 / 로비·세션 3차 전부)
- ⏳ 엔드투엔드(실서버 통신 / 세션 방)는 에디터 세팅 후 수동 검증 필요

## 추후 작업 (범위 밖)

- 실제 게임플레이(Room 이후), 방장 위임, 방 비밀번호
- 인터넷 매치메이킹(Steam/EOS)
- 토큰 디스크 영속화(자동 로그인), refresh 토큰
