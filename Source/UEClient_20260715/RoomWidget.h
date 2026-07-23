#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RoomWidget.generated.h"

class UPanelWidget;
class UButton;

/**
 * 방 안 위젯. 접속한 모든 플레이어의 닉네임/준비 상태를 표시한다.
 * BP 위젯에서 아래 이름의 자식을 배치하면 자동 연결된다:
 *   PlayerListBox (VerticalBox 등 Panel), LeaveButton (Button),
 *   ReadyButton / StartButton (Button, 선택 — 클라/호스트 전용)
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

	/** 클라이언트 전용: 준비 토글. 호스트에게는 숨긴다. */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton* ReadyButton;

	/** 호스트 전용: 게임 시작. 호스트 제외 전원 준비 시에만 활성. */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton* StartButton;

private:
	UFUNCTION()
	void HandleLeaveClicked();

	UFUNCTION()
	void HandleReadyClicked();

	UFUNCTION()
	void HandleStartClicked();

	void RefreshPlayerList();

	/** 이 위젯의 소유 플레이어가 리슨서버 호스트인지. */
	bool IsHost() const;

	FTimerHandle RefreshTimer;

	/** 로컬 준비 상태 (토글용). */
	bool bLocalReady = false;
};
