#include "RoomListEntryWidget.h"

#include "Components/TextBlock.h"
#include "Components/Button.h"

void URoomListEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (JoinButton)
	{
		JoinButton->OnClicked.AddDynamic(this, &URoomListEntryWidget::HandleJoinClicked);
	}
}

void URoomListEntryWidget::NativeDestruct()
{
	if (JoinButton)
	{
		JoinButton->OnClicked.RemoveDynamic(this, &URoomListEntryWidget::HandleJoinClicked);
	}

	Super::NativeDestruct();
}

void URoomListEntryWidget::Setup(const FRoomInfo& InInfo)
{
	Info = InInfo;

	if (RoomNameText)
	{
		RoomNameText->SetText(FText::FromString(Info.RoomName));
	}
	if (HostNameText)
	{
		HostNameText->SetText(FText::FromString(Info.HostName));
	}
	if (CountText)
	{
		CountText->SetText(FText::FromString(FString::Printf(TEXT("%d/%d"), Info.CurrentPlayers, Info.MaxPlayers)));
	}
	if (JoinButton)
	{
		JoinButton->SetIsEnabled(!Info.bIsFull);
	}
}

void URoomListEntryWidget::HandleJoinClicked()
{
	if (Info.bIsFull)
	{
		return;
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UGameSessionSubsystem* Sessions = GI->GetSubsystem<UGameSessionSubsystem>())
		{
			Sessions->JoinRoomById(Info.RoomId);
		}
	}
}
