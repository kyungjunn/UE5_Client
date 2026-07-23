#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InGamePlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
class UUserWidget;
class UInGameChatWidget;
class ANpcActor;

/**
 * InGame 레벨 컨트롤러. NPC 반경 안에서 F(IA_Interact)로 채팅 UI 를 토글한다.
 * 채팅이 열려 있는 동안 이동/시점 매핑 컨텍스트를 제거해 움직임을 차단한다.
 */
UCLASS()
class AInGamePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	/** NPC 반경 진입 시 NpcActor 가 호출 — "[F] 대화 시작" 프롬프트 표시. */
	void SetNearbyNpc(ANpcActor* Npc);

	/** NPC 반경 이탈 시 호출 — 프롬프트 숨김 + 열려 있던 채팅 닫기. */
	void ClearNearbyNpc(ANpcActor* Npc);

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	/** F 키 액션 (BP 기본값: IA_Interact). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InGame")
	UInputAction* InteractAction;

	/** F 매핑 컨텍스트 (BP 기본값: IMC_Interact). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InGame")
	UInputMappingContext* InteractMappingContext;

	/** NPC 채팅 위젯 클래스 (WBP_InGameChat). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InGame")
	TSubclassOf<UInGameChatWidget> ChatWidgetClass;

	/** "[F] 대화 시작" 프롬프트 위젯 클래스 (WBP_InteractPrompt). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InGame")
	TSubclassOf<UUserWidget> PromptWidgetClass;

private:
	void HandleInteract();
	void OpenChat();
	void CloseChat();

	/** 이동/시점 매핑 컨텍스트(IMC_Default 등)를 제거/복원한다. */
	void SetMovementInputEnabled(bool bEnabled);

	UPROPERTY()
	ANpcActor* NearbyNpc = nullptr;

	UPROPERTY()
	UUserWidget* PromptWidget = nullptr;

	UPROPERTY()
	UInGameChatWidget* ChatWidget = nullptr;
};
