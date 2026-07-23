#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LobbyAssetGenerator.generated.h"

/**
 * 에디터 전용 도구. Lobby 관련 BP/위젯 에셋(WBP_RoomEntry / WBP_Room / WBP_Lobby /
 * BP_LobbyPC)을 코드로 생성하고 BindWidget 자식·기본값까지 연결한다.
 * Python 커맨드릿에서 호출: unreal.LobbyAssetGenerator.generate_lobby_assets()
 * (생성 후 저장은 호출측에서 EditorAssetLibrary.save_directory 로 수행)
 */
UCLASS()
class ULobbyAssetGenerator : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "LobbyGen")
	static void GenerateLobbyAssets();

	/**
	 * Room 맵용 배선을 생성한다.
	 *   BP_RoomPC (ALobbyPlayerController, LobbyWidgetClass=WBP_Room)
	 *   BP_RoomGM (ALobbyGameMode, PlayerControllerClass=BP_RoomPC)
	 * (Room 맵 자체와 GameMode Override 지정은 호출측 Python 에서 수행)
	 */
	UFUNCTION(BlueprintCallable, Category = "LobbyGen")
	static void GenerateRoomWiring();

	/**
	 * NPC 채팅 에셋을 생성한다.
	 *   WBP_NpcChat (UNpcChatWidget: 초상화/이름 + 대화 로그 + 입력/전송)
	 *   WBP_Lobby 의 NpcChatWidgetClass = WBP_NpcChat 배선
	 * (WBP_Lobby 레이아웃의 우측 ChatPanelSlot 은 GenerateLobbyAssets 가 만든다)
	 */
	UFUNCTION(BlueprintCallable, Category = "LobbyGen")
	static void GenerateNpcChatAssets();

	/**
	 * InGame 에셋을 생성한다.
	 *   IA_Interact / IMC_Interact (EnhancedInput, F 키)
	 *   WBP_InGameChat (하단 반투명 채팅) / WBP_InteractPrompt ("[F] 대화 시작")
	 *   BP_InGamePC (AInGamePlayerController + 위 에셋 배선)
	 *   BP_InGameGM (AInGameGameMode, Pawn=BP_ThirdPersonCharacter, PC=BP_InGamePC)
	 * (InGame 맵 복제/NPC 배치는 호출측 Python 에서 수행)
	 */
	UFUNCTION(BlueprintCallable, Category = "LobbyGen")
	static void GenerateInGameAssets();
};
