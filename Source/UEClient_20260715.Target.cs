using UnrealBuildTool;
using System.Collections.Generic;

public class UEClient_20260715Target : TargetRules
{
	public UEClient_20260715Target(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("UEClient_20260715");
	}
}
