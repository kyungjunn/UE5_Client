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

	/** 로컬 컨트롤러 BeginPlay 시 viewport에 띄울 위젯 클래스 (BP 기본값에서 WBP_Lobby 지정). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby")
	TSubclassOf<UUserWidget> LobbyWidgetClass;

public:
	/** 방에서 준비 상태를 서버에 알린다 (클라이언트 전용 UI 에서 호출). */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Lobby")
	void ServerSetReady(bool bReady);

	/** 호스트가 게임 시작을 요청한다. 호스트 제외 전원 준비 시 InGame 으로 ServerTravel. */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Lobby")
	void ServerRequestStartGame();

private:
	/** 로컬 클라이언트가 자신의 닉네임을 서버에 알린다. */
	UFUNCTION(Server, Reliable)
	void ServerSetNickname(const FString& InNickname);
};
