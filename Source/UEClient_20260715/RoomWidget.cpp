#include "RoomWidget.h"

#include "LobbyPlayerState.h"
#include "LobbyPlayerController.h"
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
	if (ReadyButton)
	{
		ReadyButton->OnClicked.AddDynamic(this, &URoomWidget::HandleReadyClicked);
		ReadyButton->SetVisibility(IsHost() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
	if (StartButton)
	{
		StartButton->OnClicked.AddDynamic(this, &URoomWidget::HandleStartClicked);
		StartButton->SetVisibility(IsHost() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
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
	if (ReadyButton)
	{
		ReadyButton->OnClicked.RemoveDynamic(this, &URoomWidget::HandleReadyClicked);
	}
	if (StartButton)
	{
		StartButton->OnClicked.RemoveDynamic(this, &URoomWidget::HandleStartClicked);
	}

	Super::NativeDestruct();
}

bool URoomWidget::IsHost() const
{
	const APlayerController* PC = GetOwningPlayer();
	return PC && PC->HasAuthority();
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

	bool bAllOthersReady = true;
	for (const APlayerState* PS : GameState->PlayerArray)
	{
		const ALobbyPlayerState* LobbyPS = Cast<ALobbyPlayerState>(PS);
		if (!LobbyPS)
		{
			continue;
		}

		// 호스트 자신은 준비 대상에서 제외하고 나머지가 모두 준비됐는지 본다.
		const bool bIsSelf = (GetOwningPlayer() && PS == GetOwningPlayer()->PlayerState);
		if (!bIsSelf && !LobbyPS->bIsReady)
		{
			bAllOthersReady = false;
		}

		UTextBlock* Line = NewObject<UTextBlock>(this);
		FString Nickname = LobbyPS->Nickname.IsEmpty() ? TEXT("(닉네임 없음)") : LobbyPS->Nickname;
		if (LobbyPS->bIsReady)
		{
			Nickname += TEXT("  [준비완료]");
		}
		Line->SetText(FText::FromString(Nickname));
		PlayerListBox->AddChild(Line);
	}

	// 호스트: 자신을 제외한 전원이 준비됐을 때만 시작 버튼 활성 (혼자면 항상 가능).
	if (StartButton && IsHost())
	{
		StartButton->SetIsEnabled(bAllOthersReady);
	}
}

void URoomWidget::HandleReadyClicked()
{
	bLocalReady = !bLocalReady;
	if (ALobbyPlayerController* PC = Cast<ALobbyPlayerController>(GetOwningPlayer()))
	{
		PC->ServerSetReady(bLocalReady);
	}
}

void URoomWidget::HandleStartClicked()
{
	if (ALobbyPlayerController* PC = Cast<ALobbyPlayerController>(GetOwningPlayer()))
	{
		PC->ServerRequestStartGame();
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
