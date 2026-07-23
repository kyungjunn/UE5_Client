#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "InGameGameMode.generated.h"

/**
 * InGame 레벨 게임모드. DefaultPawn(BP_ThirdPersonCharacter)/PC(BP_InGamePC)는
 * BP_InGameGM 에서 지정한다.
 */
UCLASS()
class AInGameGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AInGameGameMode();
};
