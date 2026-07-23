#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NpcChatSubsystem.h"
#include "NpcChatWidget.generated.h"

class UTextBlock;
class UEditableTextBox;
class UButton;
class UScrollBox;
class UBorder;

/**
 * NPC 채팅 패널. BP 위젯에서 아래 이름의 자식을 배치하면 자동 연결된다:
 *   PortraitBorder (Border), NpcInitialText / NpcNameText (TextBlock),
 *   ChatLogBox (ScrollBox), ChatInput (EditableTextBox), SendButton (Button)
 */
UCLASS()
class UNpcChatWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UNpcChatWidget(const FObjectInitializer& ObjectInitializer);

	/** 생성 직후(AddToViewport 전) NPC 프로필 주입용. */
	void SetNpcProfile(const FNpcProfile& InProfile) { NpcProfile = InProfile; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** 이 패널이 담당하는 NPC. 기본값: 세라 (BP 에서 변경 가능). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NpcChat")
	FNpcProfile NpcProfile;

	/** 플레이스홀더 초상화 (배경색 = NpcProfile.PortraitColor). */
	UPROPERTY(meta = (BindWidget))
	UBorder* PortraitBorder;

	/** 초상화 안 이니셜 (이름 첫 글자). */
	UPROPERTY(meta = (BindWidget))
	UTextBlock* NpcInitialText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* NpcNameText;

	UPROPERTY(meta = (BindWidget))
	UScrollBox* ChatLogBox;

	UPROPERTY(meta = (BindWidget))
	UEditableTextBox* ChatInput;

	UPROPERTY(meta = (BindWidget))
	UButton* SendButton;

private:
	UFUNCTION()
	void HandleSendClicked();

	UFUNCTION()
	void HandleInputCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	UFUNCTION()
	void HandleChatResponse(bool bSuccess, const FString& NpcName, const FString& Message);

	void SendCurrentInput();

	/** "화자: 내용" 한 줄을 로그에 추가하고 스크롤을 내린다. */
	UTextBlock* AppendChatLine(const FString& Speaker, const FString& Message, const FLinearColor& Color);

	UNpcChatSubsystem* GetChat() const;

	/** 응답 대기 중 "..." 표시 라인 — 응답이 오면 내용으로 교체된다. */
	UPROPERTY()
	UTextBlock* PendingLine = nullptr;
};
