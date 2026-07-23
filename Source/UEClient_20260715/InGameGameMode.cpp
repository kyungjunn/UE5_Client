#include "InGameGameMode.h"

#include "LobbyPlayerState.h"

AInGameGameMode::AInGameGameMode()
{
	// Room 에서 넘어온 닉네임 복제를 InGame 에서도 유지한다.
	PlayerStateClass = ALobbyPlayerState::StaticClass();
}
