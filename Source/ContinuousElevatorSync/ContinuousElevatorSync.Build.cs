using UnrealBuildTool;

public class ContinuousElevatorSync : ModuleRules
{
	public ContinuousElevatorSync(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp20;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"FactoryGame",
			"SML",
			"DummyHeaders"
		});
	}
}
