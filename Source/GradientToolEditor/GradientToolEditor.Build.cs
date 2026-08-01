// Copyright (c) Jared Taylor

using UnrealBuildTool;

public class GradientToolEditor : ModuleRules
{
	public GradientToolEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"GradientTool",
				"Slate",
				"SlateCore",
				"InputCore",
				"UnrealEd",
				"AssetDefinition",
				"AssetRegistry",
				"AssetTools",
				"PropertyEditor",
				"ToolMenus",
				"AppFramework",
			}
		);
	}
}
