#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameSessionSubsystem.h"
#include "RoomListEntryWidget.generated.h"

class UTextBlock;
class UButton;

/**
 * RoomList 한 줄. BP 위젯에서 아래 이름의 자식을 배치하면 자동 연결된다:
 *   RoomNameText, HostNameText, CountText (TextBlock), JoinButton (Button)
 */
UCLASS()
class URoomListEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 방 정보로 표시를 갱신한다. 꽉 찬 방이면 Join 버튼을 비활성화한다. */
	UFUNCTION(BlueprintCallable, Category = "Session")
	void Setup(const FRoomInfo& InInfo);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* RoomNameText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* HostNameText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* CountText;

	UPROPERTY(meta = (BindWidget))
	UButton* JoinButton;

private:
	UFUNCTION()
	void HandleJoinClicked();

	FRoomInfo Info;
};
