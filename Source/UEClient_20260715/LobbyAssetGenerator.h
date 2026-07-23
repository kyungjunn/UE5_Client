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
};
