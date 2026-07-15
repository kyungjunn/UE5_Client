#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LobbyPlayerController.generated.h"

/** 로컬 GameInstance 에 저장된 닉네임을 서버로 전달해 PlayerState 에 반영한다. */
UCLASS()
class ALobbyPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

private:
	/** 로컬 클라이언트가 자신의 닉네임을 서버에 알린다. */
	UFUNCTION(Server, Reliable)
	void ServerSetNickname(const FString& InNickname);
};
