#include "LobbyGameMode.h"

#include "LobbyPlayerController.h"
#include "LobbyPlayerState.h"

ALobbyGameMode::ALobbyGameMode()
{
	PlayerControllerClass = ALobbyPlayerController::StaticClass();
	PlayerStateClass = ALobbyPlayerState::StaticClass();
	DefaultPawnClass = nullptr;
}
