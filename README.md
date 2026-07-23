# UE5_Client

언리얼 엔진 5.7 멀티플레이 클라이언트. **계정 백엔드(FastAPI + MySQL, Ubuntu)** 와 REST 통신,
**OnlineSubsystem(NULL/LAN) 리슨서버** 기반 로비/방/인게임, 그리고 **로컬 LLM(llama.cpp + Gemma)** 과
대화하는 NPC 채팅 시스템을 C++로 구현했다.

기존 블루프린트 전용 프로젝트를 C++ 프로젝트로 전환해 진행했다.

---

## 주요 기능

### 1. 계정 (서버 통신)
회원가입 · 로그인 · 내 정보 조회 · 회원탈퇴. 로그인 시 받은 JWT를 메모리에 보관한다.

### 2. 로그인 / 회원가입 UI
`UAccountSubsystem`을 호출하고 결과를 UI에 반영하는 UserWidget C++ 베이스. 화면 전환(Signup ↔ Login) 포함.

### 3. 로비 / 세션(방) 시스템
로그인 성공 → Lobby 이동, 닉네임 표시 → 방 생성 / 목록 조회 / 참가 (최대 4인, LAN 리슨서버).
방에서는 전원 닉네임·준비 상태를 확인하고, **클라 전원 준비 → 호스트가 시작** → InGame으로 동반 이동.

### 4. NPC 채팅 (로컬 LLM)
llama.cpp OpenAI 호환 서버의 Gemma와 통신. NPC별 **성격/상태 프롬프트를 조립**해 전송하고
완료 델리게이트로 응답을 받는 비동기 구조.
- **로비**: 우측 패널에서 안내 NPC "세라"와 채팅
- **InGame**: ThirdPerson으로 이동하다 필드 NPC(도우 센세이 · 데포르티스타) 반경에 들어가면
  `[F] 대화 시작` → F로 하단 반투명 채팅 UI 토글 (채팅 중 이동 차단, ESC/F 닫기)

### 5. UI 에셋 파이프라인
에디터 전용 C++ 도구(`ULobbyAssetGenerator`) + Python 커맨드릿으로 위젯/BP/맵을 코드에서 생성·배선.

---

## 아키텍처

| 항목 | 결정 |
|------|------|
| 통신 방식 | C++ (언리얼 HTTP 모듈), REST/JSON |
| 계정 서버 | FastAPI + MySQL (Ubuntu), 기본 `http://192.168.0.106:8088` (`Account.SetBaseUrl`로 변경) |
| LLM 서버 | llama.cpp `/v1/chat/completions` + Gemma, 기본 `http://127.0.0.1:8080` (`Npc.SetLlmUrl`) |
| 계정/채팅 아키텍처 | `UGameInstanceSubsystem` (BlueprintCallable + 콘솔 명령 + 델리게이트) |
| 토큰 보관 | JWT 메모리 보관 (재시작 시 소멸) |
| OnlineSubsystem | NULL (LAN) — 리슨 서버, World 기반 조회로 PIE 다중창 지원 |
| 레벨 구조 | Login → Lobby → Room(대기방) → InGame(ThirdPerson) |
| 입력 | EnhancedInput (`IA_Interact` F 키), 채팅 중 이동 IMC 제거 |
| 닉네임 | `UGameInstance` 캐시 + `LobbyPlayerState` 복제 |

---

## 레벨 흐름

```
[Login] --로그인 성공(GetMe)--> [Lobby: 프로필 + 방만들기 + RoomList + 세라 채팅]
  방 만들기 → CreateSession(max 4) → ServerTravel("Room?listen")
  방 참가   → JoinSession → ClientTravel → [Room (listen server)]
[Room] 닉네임/준비 목록 → 클라 전원 준비 + 호스트 시작 → ServerTravel("InGame?listen")
[InGame] ThirdPerson 이동 → NPC 반경 → [F] 대화 시작 → LLM 채팅
```

---

## 서버 계약

### 계정 API (Bearer JWT, HS256)

| API | 요청 본문 | 성공 응답 | 실패 |
|-----|-----------|-----------|------|
| POST `/auth/register` | email, nickname(2~50), password(8~128) | 201 (id, email, nickname, created_at) | 409 / 422 `detail` |
| POST `/auth/login` | email, password | 200 (access_token, token_type) | 401 `detail` |
| GET `/auth/me` | (Bearer) | 200 (id, email, nickname, created_at) | 401 |
| DELETE `/auth/withdraw` | (Bearer) | 204 | 401 |

### LLM API
`POST /v1/chat/completions` — messages(system: NPC 성격/상태 지침 + user), max_tokens 1024.
응답 `choices[0].message.content` 사용 (thinking 모델은 `--reasoning-budget 0` 권장).

---

## 프로젝트 구조

```
Source/UEClient_20260715/
  AccountSubsystem.h/.cpp           # 계정 서버 통신 (핵심)
  AuthUserWidgetBase.h/.cpp         # 로그인/가입 위젯 공통 베이스
  LoginUserWidget.h/.cpp            # 로그인 위젯 (성공 → GetMe → Lobby)
  RegisterUserWidget.h/.cpp         # 회원가입 위젯
  UEClientGameInstance.h/.cpp       # 닉네임 캐시
  GameSessionSubsystem.h/.cpp       # 세션: Host/Find/Join/Leave (World 기반 OSS)
  LobbyGameMode.h/.cpp              # Room 게임모드
  LobbyPlayerController.h/.cpp      # 위젯 표시 + 닉네임/준비/시작 RPC
  LobbyPlayerState.h/.cpp           # Nickname, bIsReady 복제
  LobbyWidget.h/.cpp                # 로비 (방 목록 + NPC 채팅 패널)
  RoomListEntryWidget.h/.cpp        # 방 목록 한 줄
  RoomWidget.h/.cpp                 # 대기방 (준비/시작/나가기)
  NpcChatSubsystem.h/.cpp           # LLM 통신 + FNpcProfile + 프롬프트 조립
  NpcChatWidget.h/.cpp              # 채팅 UI 베이스 (로비 세라)
  InGameChatWidget.h/.cpp           # InGame 하단 채팅 (ESC 닫기)
  InGameGameMode.h/.cpp             # InGame 게임모드
  InGamePlayerController.h/.cpp     # F 상호작용/채팅 토글/이동 차단
  NpcActor.h/.cpp                   # 필드 NPC (메시+반경+프로필+애니)
  LobbyAssetGenerator.h/.cpp        # 에디터 전용 에셋 생성 도구
Content/
  Login/, Lobby/, InGame/           # 생성된 BP/위젯 (WBP_Lobby, WBP_NpcChat, BP_InGameGM 등)
  Maps/  Login, Lobby, Room, InGame/InGame
  Input/ IMC_Default, IA_* + IA_Interact/IMC_Interact (F 키)
  ThirdPerson/, Characters/         # ThirdPerson 템플릿
  Assets/alstra_infinite/CityFolks  # NPC 캐릭터 팩 (도우 센세이, 데포르티스타)
Config/DefaultEngine.ini            # GameInstanceClass, OnlineSubsystem=Null
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

> 신규 UClass·플러그인 추가 시 Live Coding 반영 불가 → **에디터 종료 후** 빌드.

### 에셋 재생성 (선택)
위젯/BP/InGame 맵은 커맨드릿으로 재생성 가능 (에디터 종료 상태에서):
`UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script=<드라이버.py>` 가
`unreal.LobbyAssetGenerator.generate_*()` 를 호출하고 `save_directory(only_if_is_dirty=False)` 로 저장한다.

---

## 사용법

### 콘솔 명령어 (PIE 콘솔, 백틱 `` ` `` 키)
```
Account.Register <email> <nickname> <password>
Account.Login <email> <password>
Account.Me / Account.Withdraw / Account.SetBaseUrl <url>
Npc.Chat <message>            # 기본 NPC(세라)에게 전송
Npc.SetLlmUrl <url>
```
결과는 Output Log에 `[Account]` / `[NpcChat]` 로 출력.

### InGame 조작
- WASD/마우스: ThirdPerson 이동·시점 / Space: 점프
- NPC 반경 진입 시 `[F] 대화 시작` 표시 → **F**: 채팅 열기/닫기(입력창 포커스 없을 때), **ESC**: 항상 닫기
- 채팅 중에는 이동·점프·시점 입력 차단

---

## 테스트

1. 계정 서버: Ubuntu에서 `uvicorn app.main:app --host 0.0.0.0 --port 8088` (MySQL 기동 상태)
2. LLM 서버: llama.cpp `llama-server` + Gemma (포트 8080, `--reasoning-budget 0` 권장)
3. 에디터 Play: Number of Players = **2**, Net Mode = **Play As Listen Server**
4. 로그인 → 로비(세라 채팅) → 방 생성/참가 → 준비 → 시작 → InGame → NPC와 F 채팅

> LLM 주소가 `127.0.0.1`이므로 다른 PC의 클라이언트는 LLM 접근 불가(같은 PC 다중 실행은 정상).

---

## 상태

- ✅ 빌드 성공 / 실서버 계정 API(가입·로그인·내정보) / 세션 방 생성·참가 / 로비·InGame LLM 채팅 / NPC 애니메이션 — 전부 검증 완료
- 상세 검증 내역·해결 이슈는 [작업내역.md](작업내역.md) 참고

## 추후 작업 (범위 밖)

- 실제 게임플레이(InGame 콘텐츠), 방장 위임, 방 비밀번호
- 인터넷 매치메이킹(Steam/EOS)
- 토큰 디스크 영속화(자동 로그인), refresh 토큰
- NPC 대화 히스토리(멀티턴), InGame → 로비 복귀 UI
