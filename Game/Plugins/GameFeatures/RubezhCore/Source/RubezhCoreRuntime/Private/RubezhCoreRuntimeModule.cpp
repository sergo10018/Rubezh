// РУБЕЖ: Земля после разлома

#include "RubezhCoreRuntimeModule.h"

#include "AbilitySystem/Executions/LyraDamageExecution.h"
#include "Items/RubezhInventoryComponent.h"
#include "Modules/ModuleManager.h"
#include "Player/LyraPlayerSpawningManagerComponent.h"
#include "Settlement/RubezhSafeZone.h"
#include "Weapons/LyraGameplayAbility_RangedWeapon.h"

DEFINE_LOG_CATEGORY(LogRubezh);

void FRubezhCoreRuntimeModule::StartupModule()
{
	UE_LOG(LogRubezh, Log, TEXT("RubezhCoreRuntime module started"));
	// Броня и одежда уменьшают урон от попаданий: расчёт урона Lyra спрашивает множитель у снаряжения цели.
	ULyraDamageExecution::GetIncomingDamageMultiplier().BindStatic(&URubezhInventoryComponent::GetIncomingDamageMultiplier);
	// Оружие: урон из data/weapons.csv, расход патронов, износ, осечки и отдача.
	ULyraDamageExecution::GetWeaponDamageOverride().BindStatic(&URubezhInventoryComponent::GetWeaponDamage);
	ULyraGameplayAbility_RangedWeapon::GetWeaponShotCommitted().BindStatic(&URubezhInventoryComponent::HandleShotCommitted);
	// Поселение: игроки появляются внутри, боты снаружи.
	ULyraPlayerSpawningManagerComponent::GetPlayerStartFilter().BindStatic(&ARubezhSafeZone::FilterPlayerStarts);
}

void FRubezhCoreRuntimeModule::ShutdownModule()
{
	ULyraDamageExecution::GetIncomingDamageMultiplier().Unbind();
	ULyraDamageExecution::GetWeaponDamageOverride().Unbind();
	ULyraGameplayAbility_RangedWeapon::GetWeaponShotCommitted().Unbind();
	ULyraPlayerSpawningManagerComponent::GetPlayerStartFilter().Unbind();
}

IMPLEMENT_MODULE(FRubezhCoreRuntimeModule, RubezhCoreRuntime)
