#include "NpcChatWidget.h"

#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/Border.h"

namespace
{
	const FLinearColor ColorPlayerLine(0.85f, 0.9f, 1.f, 1.f);
	const FLinearColor ColorNpcLine(0.95f, 0.85f, 1.f, 1.f);
	const FLinearColor ColorErrorLine(1.f, 0.45f, 0.45f, 1.f);
}

UNpcChatWidget::UNpcChatWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NpcProfile = UNpcChatSubsystem::MakeDefaultProfile();
}

void UNpcChatWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 프로필 반영: 이름 / 이니셜 / 초상화 색
	if (NpcNameText)
	{
		NpcNameText->SetText(FText::FromString(NpcProfile.Name));
	}
	if (NpcInitialText)
	{
		NpcInitialText->SetText(FText::FromString(NpcProfile.Name.Left(1)));
	}
	if (PortraitBorder)
	{
		PortraitBorder->SetBrushColor(NpcProfile.PortraitColor);
	}

	if (SendButton)
	{
		SendButton->OnClicked.AddDynamic(this, &UNpcChatWidget::HandleSendClicked);
	}
	if (ChatInput)
	{
		ChatInput->OnTextCommitted.AddDynamic(this, &UNpcChatWidget::HandleInputCommitted);
	}
	if (UNpcChatSubsystem* Chat = GetChat())
	{
		Chat->OnChatResponseReceived.AddDynamic(this, &UNpcChatWidget::HandleChatResponse);
	}

	AppendChatLine(NpcProfile.Name, TEXT("안녕하세요! 무엇이든 물어보세요."), ColorNpcLine);
}

void UNpcChatWidget::NativeDestruct()
{
	if (SendButton)
	{
		SendButton->OnClicked.RemoveDynamic(this, &UNpcChatWidget::HandleSendClicked);
	}
	if (ChatInput)
	{
		ChatInput->OnTextCommitted.RemoveDynamic(this, &UNpcChatWidget::HandleInputCommitted);
	}
	if (UNpcChatSubsystem* Chat = GetChat())
	{
		Chat->OnChatResponseReceived.RemoveDynamic(this, &UNpcChatWidget::HandleChatResponse);
	}

	Super::NativeDestruct();
}

UNpcChatSubsystem* UNpcChatWidget::GetChat() const
{
	if (UGameInstance* GI = GetGameInstance())
	{
		return GI->GetSubsystem<UNpcChatSubsystem>();
	}
	return nullptr;
}

void UNpcChatWidget::HandleSendClicked()
{
	SendCurrentInput();
}

void UNpcChatWidget::HandleInputCommitted(const FText& /*Text*/, ETextCommit::Type CommitMethod)
{
	if (CommitMethod == ETextCommit::OnEnter)
	{
		SendCurrentInput();

		// Enter 후 계속 입력할 수 있도록 포커스 유지.
		if (ChatInput)
		{
			ChatInput->SetKeyboardFocus();
		}
	}
}

void UNpcChatWidget::SendCurrentInput()
{
	if (!ChatInput || PendingLine)
	{
		return; // 응답 대기 중에는 새 전송을 막는다.
	}

	const FString Message = ChatInput->GetText().ToString().TrimStartAndEnd();
	if (Message.IsEmpty())
	{
		return;
	}
	ChatInput->SetText(FText::GetEmpty());

	AppendChatLine(TEXT("나"), Message, ColorPlayerLine);

	if (SendButton)
	{
		SendButton->SetIsEnabled(false);
	}
	PendingLine = AppendChatLine(NpcProfile.Name, TEXT("..."), ColorNpcLine);

	if (UNpcChatSubsystem* Chat = GetChat())
	{
		Chat->SendChatMessage(NpcProfile, Message);
	}
}

void UNpcChatWidget::HandleChatResponse(bool bSuccess, const FString& /*NpcName*/, const FString& Message)
{
	if (SendButton)
	{
		SendButton->SetIsEnabled(true);
	}

	const FString Line = FString::Printf(TEXT("%s: %s"), *NpcProfile.Name, *Message);
	if (PendingLine)
	{
		PendingLine->SetText(FText::FromString(Line));
		PendingLine->SetColorAndOpacity(FSlateColor(bSuccess ? ColorNpcLine : ColorErrorLine));
		PendingLine = nullptr;
	}
	else
	{
		AppendChatLine(NpcProfile.Name, Message, bSuccess ? ColorNpcLine : ColorErrorLine);
	}

	if (ChatLogBox)
	{
		ChatLogBox->ScrollToEnd();
	}
}

UTextBlock* UNpcChatWidget::AppendChatLine(const FString& Speaker, const FString& Message, const FLinearColor& Color)
{
	if (!ChatLogBox)
	{
		return nullptr;
	}

	UTextBlock* Line = NewObject<UTextBlock>(this);
	Line->SetText(FText::FromString(FString::Printf(TEXT("%s: %s"), *Speaker, *Message)));
	Line->SetAutoWrapText(true);
	Line->SetColorAndOpacity(FSlateColor(Color));
	FSlateFontInfo Font = Line->GetFont();
	Font.Size = 18;
	Line->SetFont(Font);

	ChatLogBox->AddChild(Line);
	ChatLogBox->ScrollToEnd();
	return Line;
}
