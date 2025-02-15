using UnrealBuildTool;

public class OGAsyncTests : ModuleRules
{
	public OGAsyncTests(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"FunctionalTesting",
				"OGAsync"
			}
		);
	}
}