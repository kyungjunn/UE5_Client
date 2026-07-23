#include "InGameChatWidget.h"

FReply UInGameChatWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// 입력창 타이핑 중에도 ESC 는 항상 채팅을 닫는다.
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		OnCloseRequested.ExecuteIfBound();
		return FReply::Handled();
	}
	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}
