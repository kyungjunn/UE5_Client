#include "LobbyWidget.h"

#include "RoomListEntryWidget.h"
#include "UEClientGameInstance.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"

void ULobbyWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (NicknameText)
	{
		FString Nickname;
		if (const UUEClientGameInstance* GI = Cast<UUEClientGameInstance>(GetGameInstance()))
		{
			Nickname = GI->GetPlayerNickname();
		}
		NicknameText->SetText(FText::FromString(Nickname));
	}

	if (CreateRoomButton)
	{
		CreateRoomButton->OnClicked.AddDynamic(this, &ULobbyWidget::HandleCreateRoomClicked);
	}
	if (RefreshButton)
	{
		RefreshButton->OnClicked.AddDynamic(this, &ULobbyWidget::HandleRefreshClicked);
	}

	if (UGameSessionSubsystem* Sessions = GetSessions())
	{
		Sessions->OnFindRoomsComplete.AddDynamic(this, &ULobbyWidget::HandleFindRoomsComplete);
		Sessions->FindRooms();
	}
}

void ULobbyWidget::NativeDestruct()
{
	if (CreateRoomButton)
	{
		CreateRoomButton->OnClicked.RemoveDynamic(this, &ULobbyWidget::HandleCreateRoomClicked);
	}
	if (RefreshButton)
	{
		RefreshButton->OnClicked.RemoveDynamic(this, &ULobbyWidget::HandleRefreshClicked);
	}
	if (UGameSessionSubsystem* Sessions = GetSessions())
	{
		Sessions->OnFindRoomsComplete.RemoveDynamic(this, &ULobbyWidget::HandleFindRoomsComplete);
	}

	Super::NativeDestruct();
}

UGameSessionSubsystem* ULobbyWidget::GetSessions() const
{
	if (UGameInstance* GI = GetGameInstance())
	{
		return GI->GetSubsystem<UGameSessionSubsystem>();
	}
	return nullptr;
}

void ULobbyWidget::HandleCreateRoomClicked()
{
	const FString RoomName = RoomNameInput ? RoomNameInput->GetText().ToString() : FString();
	if (RoomName.IsEmpty())
	{
		return;
	}

	if (UGameSessionSubsystem* Sessions = GetSessions())
	{
		Sessions->HostRoom(RoomName);
	}
}

void ULobbyWidget::HandleRefreshClicked()
{
	if (UGameSessionSubsystem* Sessions = GetSessions())
	{
		Sessions->FindRooms();
	}
}

void ULobbyWidget::HandleFindRoomsComplete(const TArray<FRoomInfo>& Rooms)
{
	if (!RoomListBox || !RoomEntryWidgetClass)
	{
		return;
	}

	// RoomId 기준 동기화: 새 방은 추가, 기존 방은 갱신, 검색 결과에 없는 방은 제거.
	TSet<FString> SeenRoomIds;
	for (const FRoomInfo& Room : Rooms)
	{
		SeenRoomIds.Add(Room.RoomId);
		if (URoomListEntryWidget** Found = EntryMap.Find(Room.RoomId))
		{
			(*Found)->Setup(Room);
		}
		else
		{
			URoomListEntryWidget* Entry = CreateWidget<URoomListEntryWidget>(this, RoomEntryWidgetClass);
			if (Entry)
			{
				Entry->Setup(Room);
				RoomListBox->AddChild(Entry);
				EntryMap.Add(Room.RoomId, Entry);
			}
		}
	}

	for (auto It = EntryMap.CreateIterator(); It; ++It)
	{
		if (!SeenRoomIds.Contains(It.Key()))
		{
			It.Value()->RemoveFromParent();
			It.RemoveCurrent();
		}
	}
}
