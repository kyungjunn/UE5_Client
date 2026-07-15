#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "GameSessionSubsystem.generated.h"

/** RoomList 한 줄에 표시할 방 정보. */
USTRUCT(BlueprintType)
struct FRoomInfo
{
	GENERATED_BODY()

	/** 세션 식별자 문자열. RoomList 병합/참가의 키. */
	UPROPERTY(BlueprintReadOnly, Category = "Session")
	FString RoomId;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	FString RoomName;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	FString HostName;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	int32 CurrentPlayers = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	int32 MaxPlayers = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	bool bIsFull = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHostRoomComplete, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFindRoomsComplete, const TArray<FRoomInfo>&, Rooms);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnJoinRoomComplete, bool, bSuccess);

/**
 * 언리얼 OnlineSubsystem(NULL/LAN) 세션을 래핑한다.
 * 방 생성(HostRoom) / 검색(FindRooms) / 참가(JoinRoomById) / 나가기(LeaveRoom) 을 제공한다.
 */
UCLASS()
class UGameSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	/** 방 생성 후 성공 시 Room 맵으로 ServerTravel(listen). 최대 4인. */
	UFUNCTION(BlueprintCallable, Category = "Session")
	void HostRoom(const FString& RoomName);

	/** LAN 세션 검색. 완료 시 OnFindRoomsComplete 로 결과 브로드캐스트. */
	UFUNCTION(BlueprintCallable, Category = "Session")
	void FindRooms();

	/** RoomId 로 방 참가. 성공 시 호스트 Room 맵으로 ClientTravel. */
	UFUNCTION(BlueprintCallable, Category = "Session")
	void JoinRoomById(const FString& RoomId);

	/** 현재 세션 파기 후 Lobby 맵으로 복귀. */
	UFUNCTION(BlueprintCallable, Category = "Session")
	void LeaveRoom();

	UPROPERTY(BlueprintAssignable, Category = "Session")
	FOnHostRoomComplete OnHostRoomComplete;

	UPROPERTY(BlueprintAssignable, Category = "Session")
	FOnFindRoomsComplete OnFindRoomsComplete;

	UPROPERTY(BlueprintAssignable, Category = "Session")
	FOnJoinRoomComplete OnJoinRoomComplete;

	/** 방 생성/참가 시 이동할 Room 맵 경로. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Session")
	FString RoomMapPath = TEXT("/Game/Maps/Room");

	/** 나가기 시 복귀할 Lobby 맵 경로. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Session")
	FString LobbyMapPath = TEXT("/Game/Maps/Lobby");

private:
	IOnlineSessionPtr GetSessionInterface() const;
	FString GetLocalNickname() const;

	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);

	TSharedPtr<FOnlineSessionSearch> SearchSettings;

	FDelegateHandle CreateHandle;
	FDelegateHandle FindHandle;
	FDelegateHandle JoinHandle;
	FDelegateHandle DestroyHandle;

	/** 방 생성 시 사용한 방 이름 (설정에 저장). */
	FString PendingRoomName;
};
