# UE5 Lobby / 세션(방) 시스템 설계

작성일: 2026-07-15

## 목표

로그인 성공 후 **Lobby 레벨**로 이동하여 자신의 프로필(닉네임)을 보여주고,
언리얼 OnlineSubsystem 세션을 이용해 **방 생성 / 방 참가**를 할 수 있게 한다.

## 확정된 설계 결정

| 항목 | 결정 |
|------|------|
| OnlineSubsystem | **NULL (LAN)** — 리슨 서버 모델, 외부 계정/설정 불필요 |
| 레벨 구조 | **Lobby(브라우저) 맵 + Room(실제 방) 맵 분리** |
| 프로필 표시 | **닉네임만** (로그인 후 `GetMe`로 취득) |
| 닉네임 보관 | 커스텀 `UGameInstance`에 캐시 (레벨 이동해도 유지) |

## 레벨 흐름

```
[Login 맵] --로그인 성공(GetMe)--> [Lobby 맵 (standalone)]
   Lobby: 프로필(닉네임) + 방 만들기 폼 + RoomList
     · 방 만들기 → CreateSession(max 4) → ServerTravel("/Game/Maps/Room?listen")
     · 새로고침   → FindSessions → RoomList 갱신(병합)
     · 방 참가    → JoinSession → ClientTravel → 호스트의 Room 맵
[Room 맵 (listen server)]  LobbyGameMode/PC/PlayerState
   · 접속한 모든 플레이어의 닉네임 목록 표시
   · 나가기 → DestroySession → Lobby 맵으로 복귀
```

## 생성/수정 C++ 클래스

| 클래스 | 역할 |
|--------|------|
| `UUEClientGameInstance : UGameInstance` | 로그인 닉네임 캐시 + Get/Set (신규) |
| `UGameSessionSubsystem : UGameInstanceSubsystem` | 세션 래핑: `HostRoom` / `FindRooms` / `JoinRoomById` / `LeaveRoom`, `FRoomInfo`, 완료 델리게이트 (신규) |
| `ALobbyGameMode : AGameModeBase` | Room 맵 게임모드, PC/PS 클래스 지정, 폰 없음 (신규) |
| `ALobbyPlayerController : APlayerController` | 로컬 GameInstance 닉네임을 `ServerSetNickname` RPC로 서버 전달 (신규) |
| `ALobbyPlayerState : APlayerState` | 복제되는 `Nickname` (신규) |
| `ULobbyWidget : UUserWidget` | 프로필 + 방 만들기 + RoomList 컨테이너 (신규) |
| `URoomListEntryWidget : UUserWidget` | RoomList 한 줄: 방이름/호스트/현재·최대 + Join(꽉 차면 비활성) (신규) |
| `URoomWidget : UUserWidget` | 방 안 플레이어 닉네임 목록 + 나가기 (신규) |
| `ULoginUserWidget` (수정) | 로그인 성공 → `GetMe` → 닉네임 캐시 → `OpenLevel(Lobby)` |

## FRoomInfo (BlueprintType)

`RoomId`(세션 식별자), `RoomName`, `HostName`, `CurrentPlayers`, `MaxPlayers`, `bIsFull`.
- `CurrentPlayers = MaxPlayers - NumOpenPublicConnections`
- `bIsFull = (CurrentPlayers >= MaxPlayers)`

## 세션 세부 동작

- **Host**: `NAME_GameSession`에 `NumPublicConnections=4`, `bIsLANMatch=true`, 커스텀 설정
  `ROOMNAME`(방 이름) / `HOSTNAME`(호스트 닉네임) 저장 → 생성 성공 시 `ServerTravel("Room?listen")`.
- **Find**: `FOnlineSessionSearch(bIsLanQuery=true)` → 결과에서 `ROOMNAME/HOSTNAME/인원` 파싱해 `TArray<FRoomInfo>` 브로드캐스트. 검색 결과는 서브시스템이 보관(Join에 사용).
- **Join**: `JoinRoomById(RoomId)` → 보관된 검색 결과에서 매칭 → `JoinSession` → `GetResolvedConnectString` → `ClientTravel`.
- **Leave**: `DestroySession` → 완료 시 `OpenLevel(Lobby)`.

## RoomList "업데이트되지만 사라지지 않게"

`ULobbyWidget`가 `RoomId` 기준으로 엔트리를 **병합**한다:
- 새 `RoomId` → 엔트리 추가
- 기존 `RoomId` → 정보만 갱신
- 이번 검색에 없는 `RoomId` → **목록에서 제거하지 않음** (요청 동작)

## 방 안 닉네임 복제

- `ALobbyPlayerController::BeginPlay`(로컬) → 로컬 `UUEClientGameInstance` 닉네임 읽어
  `ServerSetNickname(Nick)` (Server RPC) 호출.
- 서버가 `ALobbyPlayerState::Nickname`(Replicated) 설정 → 클라에 복제.
- `URoomWidget`가 `GameState->PlayerArray`를 순회하며 각 `LobbyPlayerState->Nickname` 표시,
  0.5초 타이머로 갱신(입장/복제 반영). 4인 규모라 타이머로 충분(단순성 우선).

## 설정 변경

- `UEClient_20260715.Build.cs`: `OnlineSubsystem`, `OnlineSubsystemUtils` 추가
- `UEClient_20260715.uproject`: `OnlineSubsystemNull`, `OnlineSubsystemUtils` 플러그인 활성화
- `Config/DefaultEngine.ini`:
  - `[/Script/EngineSettings.GameMapsSettings] GameInstanceClass=/Script/UEClient_20260715.UEClientGameInstance`
  - `[OnlineSubsystem] DefaultPlatformService=Null`

## 사용자(에디터) 수동 작업

1. `Content/Maps/Lobby.umap`, `Content/Maps/Room.umap` 생성.
2. `WBP_Lobby`(부모 `ULobbyWidget`), `WBP_RoomEntry`(부모 `URoomListEntryWidget`),
   `WBP_Room`(부모 `URoomWidget`) 생성 후 BindWidget 이름 일치 배치.
3. `WBP_Lobby`의 `Room Entry Widget Class` = `WBP_RoomEntry` 지정.
4. Room 맵 World Settings → GameMode Override = `ALobbyGameMode`(또는 BP 자식).
5. Lobby/Room 맵의 Level Blueprint(BeginPlay)에서 각 위젯 Create + AddToViewport
   (기존 Login 맵과 동일 패턴).
6. `WBP_Login`의 `Lobby Level Name` = `Lobby` 확인.

## 검증

- **컴파일**: `Build.bat UEClient_20260715Editor Win64 Development` 성공.
- **엔드투엔드**(수동): 서버 기동 → 2개 이상 Standalone 인스턴스 각각 로그인 →
  한쪽이 방 생성 → 다른 쪽 새로고침 후 방 참가 → Room 맵에서 서로 닉네임 확인 →
  꽉 찬 방 참가 불가 확인.
  - 주의: 각 인스턴스가 독립 로그인해야 닉네임이 있음(PIE 다중 창은 GameInstance 공유 이슈로 Standalone 권장).

## 범위 밖

- 실제 게임플레이(Room 이후), 방장 위임, 방 비밀번호, 인터넷 매치메이킹(Steam/EOS).
