using UnrealBuildTool;
using System.IO;

public class RxSample : ModuleRules
{
	public string ThirdPartyPath
	{
		get { return Path.GetFullPath(Path.Combine(ModuleDirectory, "../../ThirdParty/")); }
	}

	public RxSample(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "NavigationSystem", "AIModule", "Niagara" });

		bEnableUndefinedIdentifierWarnings = false;

		PrivateIncludePaths.Add(Path.Combine(ThirdPartyPath, "RxCpp"));
	}
}
