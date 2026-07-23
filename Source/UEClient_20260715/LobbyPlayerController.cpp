#include "LobbyPlayerController.h"

#include "LobbyPlayerState.h"
#include "UEClientGameInstance.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/GameStateBase.h"
#include "Engine/World.h"

namespace
{
	const TCHAR* InGameMapPath = TEXT("/Game/Maps/InGame/InGame");
}

void ALobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController())
	{
		// 로비/방 UI 를 조작할 수 있도록 커서 표시 + UI 입력 모드로 전환.
		// (리슨서버 호스트와 접속한 클라이언트 모두 각자의 로컬 컨트롤러에서 실행됨)
		bShowMouseCursor = true;
		SetInputMode(FInputModeUIOnly());

		FString Nickname;
		if (const UUEClientGameInstance* GI = Cast<UUEClientGameInstance>(GetGameInstance()))
		{
			Nickname = GI->GetPlayerNickname();
		}
		ServerSetNickname(Nickname);

		if (LobbyWidgetClass)
		{
			if (UUserWidget* Widget = CreateWidget<UUserWidget>(this, LobbyWidgetClass))
			{
				Widget->AddToViewport();
			}
		}
	}
}

void ALobbyPlayerController::ServerSetNickname_Implementation(const FString& InNickname)
{
	if (ALobbyPlayerState* LobbyPS = GetPlayerState<ALobbyPlayerState>())
	{
		LobbyPS->Nickname = InNickname;
	}
}

void ALobbyPlayerController::ServerSetReady_Implementation(bool bReady)
{
	if (ALobbyPlayerState* LobbyPS = GetPlayerState<ALobbyPlayerState>())
	{
		LobbyPS->bIsReady = bReady;
	}
}

void ALobbyPlayerController::ServerRequestStartGame_Implementation()
{
	// 리슨서버에서 서버의 로컬 컨트롤러 = 호스트. 호스트의 요청만 처리한다.
	if (!IsLocalController())
	{
		return;
	}

	UWorld* World = GetWorld();
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	if (!GameState)
	{
		return;
	}

	// 호스트(자신) 제외 전원 준비 완료여야 시작 (혼자면 조건 없이 시작 가능).
	for (const APlayerState* PS : GameState->PlayerArray)
	{
		if (PS == PlayerState)
		{
			continue;
		}
		const ALobbyPlayerState* LobbyPS = Cast<ALobbyPlayerState>(PS);
		if (LobbyPS && !LobbyPS->bIsReady)
		{
			return;
		}
	}

	World->ServerTravel(FString::Printf(TEXT("%s?listen"), InGameMapPath));
}
