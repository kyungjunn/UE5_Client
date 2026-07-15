using UnrealBuildTool;
using System.Collections.Generic;

public class UEClient_20260715EditorTarget : TargetRules
{
	public UEClient_20260715EditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("UEClient_20260715");
	}
}
