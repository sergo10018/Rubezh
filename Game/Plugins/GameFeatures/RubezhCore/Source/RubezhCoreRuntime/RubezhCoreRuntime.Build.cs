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
				"DeveloperSettings",
				"InputCore",
				"UMG",
				"CommonUI",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"GameplayTags",
				"AIModule",
				"GameplayTasks",
				"EnhancedInput",
				"NetCore",
				"Slate",
				"SlateCore",
				"CommonGame",
			}
		);
	}
}
