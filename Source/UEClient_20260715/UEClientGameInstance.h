#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "UEClientGameInstance.generated.h"

/**
 * 로그인한 유저의 닉네임을 레벨 이동 후에도 유지하기 위해 캐시하는 GameInstance.
 * 로그인 성공 후 ULoginUserWidget 이 닉네임을 저장하고, Room 맵의 PlayerController 가 읽어간다.
 */
UCLASS()
class UUEClientGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Account")
	const FString& GetPlayerNickname() const { return PlayerNickname; }

	UFUNCTION(BlueprintCallable, Category = "Account")
	void SetPlayerNickname(const FString& InNickname) { PlayerNickname = InNickname; }

private:
	UPROPERTY()
	FString PlayerNickname;
};
