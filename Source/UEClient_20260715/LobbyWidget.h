#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameSessionSubsystem.h"
#include "LobbyWidget.generated.h"

class UTextBlock;
class UEditableTextBox;
class UButton;
class UPanelWidget;
class UBorder;
class URoomListEntryWidget;

/**
 * Lobby(브라우저) 위젯. BP 위젯에서 아래 이름의 자식을 배치하면 자동 연결된다:
 *   NicknameText (TextBlock), RoomNameInput (EditableTextBox),
 *   CreateRoomButton / RefreshButton (Button), RoomListBox (ScrollBox 등 Panel)
 */
UCLASS()
class ULobbyWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* NicknameText;

	UPROPERTY(meta = (BindWidget))
	UEditableTextBox* RoomNameInput;

	UPROPERTY(meta = (BindWidget))
	UButton* CreateRoomButton;

	UPROPERTY(meta = (BindWidget))
	UButton* RefreshButton;

	/** RoomList 엔트리들이 들어가는 컨테이너 (ScrollBox/VerticalBox 등). */
	UPROPERTY(meta = (BindWidget))
	UPanelWidget* RoomListBox;

	/** RoomList 한 줄로 사용할 위젯 클래스 (WBP_RoomEntry 지정). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Session")
	TSubclassOf<URoomListEntryWidget> RoomEntryWidgetClass;

	/** NPC 채팅 패널이 들어갈 우측 슬롯 (없으면 채팅 비활성). */
	UPROPERTY(meta = (BindWidgetOptional))
	UBorder* ChatPanelSlot;

	/** 우측 패널에 생성할 NPC 채팅 위젯 클래스 (WBP_NpcChat 지정). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NpcChat")
	TSubclassOf<UUserWidget> NpcChatWidgetClass;

private:
	UFUNCTION()
	void HandleCreateRoomClicked();

	UFUNCTION()
	void HandleRefreshClicked();

	UFUNCTION()
	void HandleFindRoomsComplete(const TArray<FRoomInfo>& Rooms);

	UGameSessionSubsystem* GetSessions() const;

	/** RoomId → 엔트리 위젯. 검색에서 사라진 방도 제거하지 않고 유지한다. */
	UPROPERTY()
	TMap<FString, URoomListEntryWidget*> EntryMap;
};
