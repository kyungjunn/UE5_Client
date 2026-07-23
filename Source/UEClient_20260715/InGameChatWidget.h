#pragma once

#include "CoreMinimal.h"
#include "NpcChatWidget.h"
#include "InGameChatWidget.generated.h"

/**
 * InGame 하단 채팅 위젯. ESC 로 닫기 요청을 보낸다.
 * (F 닫기는 입력창 포커스가 없을 때 게임 입력으로 내려가 PC 가 처리한다)
 */
UCLASS()
class UInGameChatWidget : public UNpcChatWidget
{
	GENERATED_BODY()

public:
	/** 닫기 요청 (ESC). AInGamePlayerController 가 바인딩한다. */
	FSimpleDelegate OnCloseRequested;

protected:
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
};
