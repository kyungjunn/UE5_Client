using UnrealBuildTool;

public class UEClient_20260715 : ModuleRules
{
	public UEClient_20260715(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "UMG" });

		PrivateDependencyModuleNames.AddRange(new string[] { "HTTP", "Json", "Slate", "SlateCore" });
	}
}
