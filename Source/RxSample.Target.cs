using UnrealBuildTool;
using System.Collections.Generic;

public class RxSampleTarget : TargetRules
{
	public RxSampleTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		ExtraModuleNames.Add("RxSample");
	}
}
