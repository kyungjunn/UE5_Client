#include "LobbyPlayerController.h"

#include "LobbyPlayerState.h"
#include "UEClientGameInstance.h"

void ALobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController())
	{
		FString Nickname;
		if (const UUEClientGameInstance* GI = Cast<UUEClientGameInstance>(GetGameInstance()))
		{
			Nickname = GI->GetPlayerNickname();
		}
		ServerSetNickname(Nickname);
	}
}

void ALobbyPlayerController::ServerSetNickname_Implementation(const FString& InNickname)
{
	if (ALobbyPlayerState* LobbyPS = GetPlayerState<ALobbyPlayerState>())
	{
		LobbyPS->Nickname = InNickname;
	}
}
