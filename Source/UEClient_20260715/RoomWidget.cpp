#include "RoomWidget.h"

#include "LobbyPlayerState.h"
#include "GameSessionSubsystem.h"
#include "GameFramework/GameStateBase.h"
#include "Components/PanelWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "TimerManager.h"

void URoomWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (LeaveButton)
	{
		LeaveButton->OnClicked.AddDynamic(this, &URoomWidget::HandleLeaveClicked);
	}

	RefreshPlayerList();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(RefreshTimer, this, &URoomWidget::RefreshPlayerList, 0.5f, true);
	}
}

void URoomWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RefreshTimer);
	}
	if (LeaveButton)
	{
		LeaveButton->OnClicked.RemoveDynamic(this, &URoomWidget::HandleLeaveClicked);
	}

	Super::NativeDestruct();
}

void URoomWidget::RefreshPlayerList()
{
	if (!PlayerListBox)
	{
		return;
	}

	PlayerListBox->ClearChildren();

	const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (!GameState)
	{
		return;
	}

	for (const APlayerState* PS : GameState->PlayerArray)
	{
		const ALobbyPlayerState* LobbyPS = Cast<ALobbyPlayerState>(PS);
		if (!LobbyPS)
		{
			continue;
		}

		UTextBlock* Line = NewObject<UTextBlock>(this);
		const FString Nickname = LobbyPS->Nickname.IsEmpty() ? TEXT("(닉네임 없음)") : LobbyPS->Nickname;
		Line->SetText(FText::FromString(Nickname));
		PlayerListBox->AddChild(Line);
	}
}

void URoomWidget::HandleLeaveClicked()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UGameSessionSubsystem* Sessions = GI->GetSubsystem<UGameSessionSubsystem>())
		{
			Sessions->LeaveRoom();
		}
	}
}
