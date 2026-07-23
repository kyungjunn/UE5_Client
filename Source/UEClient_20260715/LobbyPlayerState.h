#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "LobbyPlayerState.generated.h"

/** 방 안에서 서로를 확인하기 위해 닉네임을 복제하는 PlayerState. */
UCLASS()
class ALobbyPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	/** 서버에서 설정 → 모든 클라이언트로 복제. */
	UPROPERTY(ReplicatedUsing = OnRep_Nickname, BlueprintReadOnly, Category = "Lobby")
	FString Nickname;

	/** 방에서 준비 완료 여부 (호스트 제외 전원 true 여야 시작 가능). */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Lobby")
	bool bIsReady = false;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UFUNCTION()
	void OnRep_Nickname();
};
