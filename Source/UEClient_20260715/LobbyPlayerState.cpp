#include "LobbyPlayerState.h"

#include "Net/UnrealNetwork.h"

void ALobbyPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALobbyPlayerState, Nickname);
}

void ALobbyPlayerState::OnRep_Nickname()
{
	// 위젯은 GameState->PlayerArray 를 타이머로 갱신하므로 별도 처리 불필요.
}
