#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LobbyGameMode.generated.h"

/** Room 맵의 게임모드. 폰 없이 PlayerController/PlayerState 만 사용하는 대기실. */
UCLASS()
class ALobbyGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ALobbyGameMode();
};
