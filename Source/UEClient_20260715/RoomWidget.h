#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RoomWidget.generated.h"

class UPanelWidget;
class UButton;

/**
 * 방 안 위젯. 접속한 모든 플레이어의 닉네임을 표시한다.
 * BP 위젯에서 아래 이름의 자식을 배치하면 자동 연결된다:
 *   PlayerListBox (VerticalBox 등 Panel), LeaveButton (Button)
 */
UCLASS()
class URoomWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	UPanelWidget* PlayerListBox;

	UPROPERTY(meta = (BindWidget))
	UButton* LeaveButton;

private:
	UFUNCTION()
	void HandleLeaveClicked();

	void RefreshPlayerList();

	FTimerHandle RefreshTimer;
};
