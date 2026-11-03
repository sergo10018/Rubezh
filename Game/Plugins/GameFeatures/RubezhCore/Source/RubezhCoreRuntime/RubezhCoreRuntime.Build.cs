// РУБЕЖ: Земля после разлома

using UnrealBuildTool;

public class RubezhCoreRuntime : ModuleRules
{
	public RubezhCoreRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"LyraGame",
				"ModularGameplay",
				"GameplayAbilities",
				"GameplayMessageRuntime",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"GameplayTags",
				"GameplayTasks",
				"EnhancedInput",
				"NetCore",
			}
		);
	}
}
