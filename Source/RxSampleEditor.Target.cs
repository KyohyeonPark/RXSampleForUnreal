using UnrealBuildTool;
using System.Collections.Generic;

public class RxSampleEditorTarget : TargetRules
{
	public RxSampleEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		ExtraModuleNames.Add("RxSample");
	}
}
