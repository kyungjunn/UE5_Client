#include "GameSessionSubsystem.h"

#include "UEClientGameInstance.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

namespace
{
	const FName KeyRoomName(TEXT("ROOMNAME"));
	const FName KeyHostName(TEXT("HOSTNAME"));
}

void UGameSessionSubsystem::Deinitialize()
{
	if (IOnlineSessionPtr Session = GetSessionInterface())
	{
		Session->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
		Session->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
		Session->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
		Session->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);
	}

	Super::Deinitialize();
}

IOnlineSessionPtr UGameSessionSubsystem::GetSessionInterface() const
{
	if (IOnlineSubsystem* OSS = IOnlineSubsystem::Get())
	{
		return OSS->GetSessionInterface();
	}
	return nullptr;
}

FString UGameSessionSubsystem::GetLocalNickname() const
{
	if (const UUEClientGameInstance* GI = Cast<UUEClientGameInstance>(GetGameInstance()))
	{
		return GI->GetPlayerNickname();
	}
	return FString();
}

void UGameSessionSubsystem::HostRoom(const FString& RoomName)
{
	IOnlineSessionPtr Session = GetSessionInterface();
	if (!Session.IsValid())
	{
		OnHostRoomComplete.Broadcast(false);
		return;
	}

	// 기존 세션이 남아 있으면 먼저 정리.
	if (Session->GetNamedSession(NAME_GameSession))
	{
		Session->DestroySession(NAME_GameSession);
	}

	PendingRoomName = RoomName;

	FOnlineSessionSettings Settings;
	Settings.NumPublicConnections = 4;
	Settings.NumPrivateConnections = 0;
	Settings.bIsLANMatch = true;
	Settings.bShouldAdvertise = true;
	Settings.bAllowJoinInProgress = true;
	Settings.bUsesPresence = false;
	Settings.bAllowJoinViaPresence = false;
	Settings.bIsDedicated = false;
	Settings.Set(KeyRoomName, RoomName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	Settings.Set(KeyHostName, GetLocalNickname(), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	CreateHandle = Session->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &UGameSessionSubsystem::OnCreateSessionComplete));

	Session->CreateSession(0, NAME_GameSession, Settings);
}

void UGameSessionSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (IOnlineSessionPtr Session = GetSessionInterface())
	{
		Session->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
	}

	OnHostRoomComplete.Broadcast(bWasSuccessful);

	if (bWasSuccessful)
	{
		if (UWorld* World = GetWorld())
		{
			World->ServerTravel(FString::Printf(TEXT("%s?listen"), *RoomMapPath));
		}
	}
}

void UGameSessionSubsystem::FindRooms()
{
	IOnlineSessionPtr Session = GetSessionInterface();
	if (!Session.IsValid())
	{
		OnFindRoomsComplete.Broadcast(TArray<FRoomInfo>());
		return;
	}

	SearchSettings = MakeShared<FOnlineSessionSearch>();
	SearchSettings->bIsLanQuery = true;
	SearchSettings->MaxSearchResults = 20;

	FindHandle = Session->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &UGameSessionSubsystem::OnFindSessionsComplete));

	Session->FindSessions(0, SearchSettings.ToSharedRef());
}

void UGameSessionSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
	if (IOnlineSessionPtr Session = GetSessionInterface())
	{
		Session->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
	}

	TArray<FRoomInfo> Rooms;
	if (bWasSuccessful && SearchSettings.IsValid())
	{
		for (const FOnlineSessionSearchResult& Result : SearchSettings->SearchResults)
		{
			FRoomInfo Info;
			Info.RoomId = Result.GetSessionIdStr();
			Result.Session.SessionSettings.Get(KeyRoomName, Info.RoomName);
			Result.Session.SessionSettings.Get(KeyHostName, Info.HostName);
			Info.MaxPlayers = Result.Session.SessionSettings.NumPublicConnections;
			Info.CurrentPlayers = Info.MaxPlayers - Result.Session.NumOpenPublicConnections;
			Info.bIsFull = (Info.CurrentPlayers >= Info.MaxPlayers);
			Rooms.Add(Info);
		}
	}

	OnFindRoomsComplete.Broadcast(Rooms);
}

void UGameSessionSubsystem::JoinRoomById(const FString& RoomId)
{
	IOnlineSessionPtr Session = GetSessionInterface();
	if (!Session.IsValid() || !SearchSettings.IsValid())
	{
		OnJoinRoomComplete.Broadcast(false);
		return;
	}

	const FOnlineSessionSearchResult* Match = SearchSettings->SearchResults.FindByPredicate(
		[&RoomId](const FOnlineSessionSearchResult& R) { return R.GetSessionIdStr() == RoomId; });

	if (!Match)
	{
		// 최신 검색 결과에 없는(사라졌거나 오래된) 방.
		OnJoinRoomComplete.Broadcast(false);
		return;
	}

	JoinHandle = Session->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &UGameSessionSubsystem::OnJoinSessionComplete));

	Session->JoinSession(0, NAME_GameSession, *Match);
}

void UGameSessionSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	IOnlineSessionPtr Session = GetSessionInterface();
	if (Session.IsValid())
	{
		Session->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
	}

	if (Result == EOnJoinSessionCompleteResult::Success && Session.IsValid())
	{
		FString ConnectString;
		if (Session->GetResolvedConnectString(NAME_GameSession, ConnectString))
		{
			if (APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController())
			{
				PC->ClientTravel(ConnectString, TRAVEL_Absolute);
				OnJoinRoomComplete.Broadcast(true);
				return;
			}
		}
	}

	OnJoinRoomComplete.Broadcast(false);
}

void UGameSessionSubsystem::LeaveRoom()
{
	IOnlineSessionPtr Session = GetSessionInterface();
	if (!Session.IsValid() || !Session->GetNamedSession(NAME_GameSession))
	{
		// 세션이 없으면 그냥 Lobby 로 복귀.
		if (UWorld* World = GetWorld())
		{
			World->ServerTravel(LobbyMapPath);
		}
		return;
	}

	DestroyHandle = Session->AddOnDestroySessionCompleteDelegate_Handle(
		FOnDestroySessionCompleteDelegate::CreateUObject(this, &UGameSessionSubsystem::OnDestroySessionComplete));

	Session->DestroySession(NAME_GameSession);
}

void UGameSessionSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (IOnlineSessionPtr Session = GetSessionInterface())
	{
		Session->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);
	}

	if (APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController())
	{
		PC->ClientTravel(LobbyMapPath, TRAVEL_Absolute);
	}
	else if (UWorld* World = GetWorld())
	{
		World->ServerTravel(LobbyMapPath);
	}
}
