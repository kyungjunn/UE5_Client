#include "LobbyPlayerController.h"

#include "LobbyPlayerState.h"
#include "UEClientGameInstance.h"
#include "Blueprint/UserWidget.h"

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
