using UnrealBuildTool;

public class UEClient_20260715 : ModuleRules
{
	public UEClient_20260715(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "UMG", "InputCore" });

		PrivateDependencyModuleNames.AddRange(new string[] { "HTTP", "Json", "Slate", "SlateCore", "OnlineSubsystem", "OnlineSubsystemUtils", "EnhancedInput" });

		// 에디터 전용: Lobby BP/위젯 에셋을 코드로 생성하는 도구(LobbyAssetGenerator)용.
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "UMGEditor", "AssetTools" });
		}
	}
}
