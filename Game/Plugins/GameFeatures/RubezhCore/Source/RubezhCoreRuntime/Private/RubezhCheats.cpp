// РУБЕЖ: Земля после разлома
// Отладочные консольные команды для плейтеста. В сборку Shipping не попадают.

#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Character/LyraHealthComponent.h"
#include "GameplayEffect.h"
#include "Items/RubezhEquipment.h"
#include "Health/RubezhHealthEffects.h"
#include "Health/RubezhInjuryComponent.h"
#include "Items/RubezhInventoryComponent.h"
#include "Items/RubezhWorldItem.h"
#include "RubezhCoreRuntimeModule.h"
#include "RubezhGameplayTags.h"

#if !UE_BUILD_SHIPPING

namespace
{
	URubezhInventoryComponent* FindLocalInventory(UWorld* World)
	{
		const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		return URubezhInventoryComponent::FindForActor(Controller);
	}

	void GiveItem(const TArray<FString>& Args, UWorld* World)
	{
		if (Args.Num() < 1)
		{
			UE_LOG(LogRubezh, Display, TEXT("Rubezh.Give <ItemId> [Count] — например: Rubezh.Give ammo_762x39 60"));
			return;
		}
		if (!World || World->GetNetMode() == NM_Client)
		{
			UE_LOG(LogRubezh, Warning, TEXT("Rubezh.Give работает только там, где есть сервер (одиночная игра или хост)"));
			return;
		}

		URubezhInventoryComponent* Inventory = FindLocalInventory(World);
		if (!Inventory)
		{
			UE_LOG(LogRubezh, Warning, TEXT("Rubezh.Give: у игрока нет рюкзака"));
			return;
		}

		const int32 Count = Args.Num() > 1 ? FCString::Atoi(*Args[1]) : 1;
		const int32 NotAdded = Inventory->AddItem(FName(*Args[0]), Count);
		UE_LOG(LogRubezh, Display, TEXT("Rubezh.Give %s ×%d: добавлено %d, не влезло %d, вес %.2f кг"),
			*Args[0], Count, Count - NotAdded, NotAdded, Inventory->GetTotalWeight());
	}

	void TakeItem(const TArray<FString>& Args, UWorld* World)
	{
		URubezhInventoryComponent* Inventory = (World && World->GetNetMode() != NM_Client) ? FindLocalInventory(World) : nullptr;
		if (!Inventory || Args.Num() < 1)
		{
			UE_LOG(LogRubezh, Display, TEXT("Rubezh.Take <ItemId> [Count] — только в одиночной игре или на хосте"));
			return;
		}

		const int32 Count = Args.Num() > 1 ? FCString::Atoi(*Args[1]) : 1;
		const int32 Removed = Inventory->RemoveItem(FName(*Args[0]), Count);
		UE_LOG(LogRubezh, Display, TEXT("Rubezh.Take %s: убрано %d, вес %.2f кг"), *Args[0], Removed, Inventory->GetTotalWeight());
	}

	void SpawnItem(const TArray<FString>& Args, UWorld* World)
	{
		APlayerController* Controller = (World && World->GetNetMode() != NM_Client) ? World->GetFirstPlayerController() : nullptr;
		APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
		if (!Pawn || Args.Num() < 1)
		{
			UE_LOG(LogRubezh, Display, TEXT("Rubezh.Spawn <ItemId> [Count] — положить предмет на землю перед собой (одиночная игра или хост)"));
			return;
		}

		const int32 Count = Args.Num() > 1 ? FCString::Atoi(*Args[1]) : 1;
		const ARubezhWorldItem* Item = ARubezhWorldItem::SpawnInFrontOf(*Pawn, FName(*Args[0]), Count);
		if (Item && !Item->FindDefinition())
		{
			UE_LOG(LogRubezh, Warning, TEXT("Rubezh.Spawn: предмета '%s' нет в таблице — лежит серая коробка"), *Args[0]);
		}
	}

	APawn* FindLocalPawnOnServer(UWorld* World)
	{
		APlayerController* Controller = (World && World->GetNetMode() != NM_Client) ? World->GetFirstPlayerController() : nullptr;
		return Controller ? Controller->GetPawn() : nullptr;
	}

	void InjurePlayer(const TArray<FString>& Args, UWorld* World)
	{
		using namespace Rubezh::Injuries;
		URubezhInjuryComponent* Injuries = URubezhInjuryComponent::FindForActor(FindLocalPawnOnServer(World));
		const FString What = Args.Num() > 0 ? Args[0].ToLower() : FString();

		FMask Mask = 0;
		if (What == TEXT("bleeding") || What == TEXT("all")) { Mask |= Bit(ERubezhInjury::Bleeding); }
		if (What == TEXT("heavy") || What == TEXT("all"))    { Mask |= Bit(ERubezhInjury::HeavyBleeding); }
		if (What == TEXT("fracture") || What == TEXT("all")) { Mask |= Bit(ERubezhInjury::LegFracture); }
		if (Mask != 0 || What == TEXT("pain"))               { Mask |= Bit(ERubezhInjury::Pain); }

		if (!Injuries || Mask == 0)
		{
			UE_LOG(LogRubezh, Display, TEXT("Rubezh.Injure bleeding|heavy|fracture|pain|all — нанести травму (одиночная игра или хост)"));
			return;
		}
		Injuries->AddInjuries(Mask);
	}

	void HurtPlayer(const TArray<FString>& Args, UWorld* World)
	{
		UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(FindLocalPawnOnServer(World));
		const float Damage = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 0.0f;
		if (!ASC || Damage <= 0.0f)
		{
			UE_LOG(LogRubezh, Display, TEXT("Rubezh.Hurt <урон> — ранить себя, как попаданием (травмы выпадают по обычным шансам)"));
			return;
		}

		const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(URubezhGE_InjuryDamage::StaticClass(), 1.0f, ASC->MakeEffectContext());
		Spec.Data->SetSetByCallerMagnitude(RubezhGameplayTags::SetByCaller_Damage, Damage);
		ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data);
	}

	void EquipItem(const TArray<FString>& Args, UWorld* World)
	{
		URubezhInventoryComponent* Inventory = (World && World->GetNetMode() != NM_Client) ? FindLocalInventory(World) : nullptr;
		if (!Inventory || Args.Num() < 1)
		{
			UE_LOG(LogRubezh, Display, TEXT("Rubezh.Equip <ItemId> — надеть (если предмета нет в рюкзаке, он сначала выдаётся)"));
			return;
		}

		const FName ItemId(*Args[0]);
		auto FindIndex = [Inventory, ItemId]()
		{
			return Inventory->GetGrid().GetEntries().IndexOfByPredicate([ItemId](const FRubezhInventoryEntry& Entry) { return Entry.ItemId == ItemId; });
		};
		if (FindIndex() == INDEX_NONE)
		{
			Inventory->AddItem(ItemId, 1);
		}
		const int32 Index = FindIndex();
		if (Index == INDEX_NONE)
		{
			UE_LOG(LogRubezh, Warning, TEXT("Rubezh.Equip: '%s' не поместился в рюкзак"), *Args[0]);
			return;
		}

		FRubezhInventoryGrid Grid = Inventory->GetGrid();
		Rubezh::Equipment::FEquipped Equipped = Inventory->GetEquippedItems();
		const Rubezh::Equipment::EResult Result = Rubezh::Equipment::Equip(Grid, Equipped, Index,
			[Inventory](FName Id) { return Inventory->FindDefinition(Id); }, URubezhInventoryComponent::GetPocketsSize());
		Inventory->ServerEquipEntry(Index);
		UE_LOG(LogRubezh, Display, TEXT("Rubezh.Equip %s: %s"), *Args[0],
			Result == Rubezh::Equipment::EResult::Ok ? TEXT("надето") : Rubezh::Equipment::Describe(Result));
	}

	void UnequipItem(const TArray<FString>& Args, UWorld* World)
	{
		URubezhInventoryComponent* Inventory = (World && World->GetNetMode() != NM_Client) ? FindLocalInventory(World) : nullptr;
		const FString Name = Args.Num() > 0 ? Args[0].ToLower() : FString();
		const ERubezhEquipSlot Slot =
			Name == TEXT("head") ? ERubezhEquipSlot::Head :
			Name == TEXT("armor") ? ERubezhEquipSlot::Armor :
			Name == TEXT("top") ? ERubezhEquipSlot::Top :
			Name == TEXT("bottom") ? ERubezhEquipSlot::Bottom :
			Name == TEXT("backpack") ? ERubezhEquipSlot::Backpack : ERubezhEquipSlot::None;
		if (!Inventory || Slot == ERubezhEquipSlot::None)
		{
			UE_LOG(LogRubezh, Display, TEXT("Rubezh.Unequip head|armor|top|bottom|backpack — снять в рюкзак"));
			return;
		}

		FRubezhInventoryGrid Grid = Inventory->GetGrid();
		Rubezh::Equipment::FEquipped Equipped = Inventory->GetEquippedItems();
		const Rubezh::Equipment::EResult Result = Rubezh::Equipment::Unequip(Grid, Equipped, Slot, {}, false,
			[Inventory](FName Id) { return Inventory->FindDefinition(Id); }, URubezhInventoryComponent::GetPocketsSize());
		Inventory->ServerUnequip(Slot, FIntPoint::ZeroValue, false, true);
		UE_LOG(LogRubezh, Display, TEXT("Rubezh.Unequip %s: %s"), *Name,
			Result == Rubezh::Equipment::EResult::Ok ? TEXT("снято") : Rubezh::Equipment::Describe(Result));
	}

	/** Попадание через расчёт урона Lyra (как от оружия) в выбранную часть тела — проверить броню. */
	void ShootSelf(const TArray<FString>& Args, UWorld* World)
	{
		APawn* Pawn = FindLocalPawnOnServer(World);
		UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn);
		const float Damage = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 0.0f;
		const FString Zone = Args.Num() > 1 ? Args[1].ToLower() : FString(TEXT("torso"));
		const FName Bone = Zone == TEXT("head") ? FName(TEXT("head")) : Zone == TEXT("arms") ? FName(TEXT("lowerarm_l"))
			: Zone == TEXT("legs") ? FName(TEXT("calf_l")) : FName(TEXT("spine_03"));
		const TSubclassOf<UGameplayEffect> DamageEffect = LoadClass<UGameplayEffect>(nullptr,
			TEXT("/Game/GameplayEffects/Damage/GE_Damage_Basic_SetByCaller.GE_Damage_Basic_SetByCaller_C"));
		const ULyraHealthComponent* Health = ULyraHealthComponent::FindHealthComponent(Pawn);
		if (!ASC || !DamageEffect || !Health || Damage <= 0.0f)
		{
			UE_LOG(LogRubezh, Display, TEXT("Rubezh.Shoot <урон> [head|torso|arms|legs] — попадание как от оружия, с учётом брони"));
			return;
		}

		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		FHitResult Hit;
		Hit.BoneName = Bone;
		Hit.HitObjectHandle = FActorInstanceHandle(Pawn);
		Hit.ImpactPoint = Pawn->GetActorLocation();
		Hit.Location = Hit.ImpactPoint;
		Context.AddHitResult(Hit, true);

		const float Before = Health->GetHealth();
		const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(DamageEffect, 1.0f, Context);
		Spec.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(TEXT("SetByCaller.Damage")), Damage);
		ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data);
		UE_LOG(LogRubezh, Display, TEXT("Rubezh.Shoot %.0f в %s (%s): здоровье %.1f → %.1f"),
			Damage, *Zone, *Bone.ToString(), Before, Health->GetHealth());
	}

	void PrintInventory(const TArray<FString>& Args, UWorld* World)
	{
		const URubezhInventoryComponent* Inventory = FindLocalInventory(World);
		if (!Inventory)
		{
			UE_LOG(LogRubezh, Warning, TEXT("Rubezh.Inventory: у игрока нет рюкзака"));
			return;
		}

		for (const ERubezhEquipSlot Slot : Rubezh::Equipment::AllSlots)
		{
			UE_LOG(LogRubezh, Display, TEXT("  [%s] %s"), Rubezh::Equipment::GetSlotName(Slot), *Inventory->GetEquipped(Slot).ToString());
		}
		const FRubezhInventoryGrid& Grid = Inventory->GetGrid();
		UE_LOG(LogRubezh, Display, TEXT("Рюкзак %dx%d, стопок %d, вес %.2f кг"),
			Grid.GetSize().X, Grid.GetSize().Y, Grid.GetEntries().Num(), Inventory->GetTotalWeight());
		for (const FRubezhInventoryEntry& Entry : Grid.GetEntries())
		{
			UE_LOG(LogRubezh, Display, TEXT("  %s ×%d в (%d,%d)%s"),
				*Entry.ItemId.ToString(), Entry.Count, Entry.Position.X, Entry.Position.Y, Entry.bRotated ? TEXT(" повёрнут") : TEXT(""));
		}
	}

	FAutoConsoleCommandWithWorldAndArgs GiveCommand(
		TEXT("Rubezh.Give"), TEXT("Положить предмет в рюкзак: Rubezh.Give <ItemId> [Count]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&GiveItem));

	FAutoConsoleCommandWithWorldAndArgs TakeCommand(
		TEXT("Rubezh.Take"), TEXT("Убрать предмет из рюкзака: Rubezh.Take <ItemId> [Count]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TakeItem));

	FAutoConsoleCommandWithWorldAndArgs SpawnCommand(
		TEXT("Rubezh.Spawn"), TEXT("Положить предмет на землю перед собой: Rubezh.Spawn <ItemId> [Count]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SpawnItem));

	FAutoConsoleCommandWithWorldAndArgs InjureCommand(
		TEXT("Rubezh.Injure"), TEXT("Нанести травму: Rubezh.Injure bleeding|heavy|fracture|pain|all"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&InjurePlayer));

	FAutoConsoleCommandWithWorldAndArgs HurtCommand(
		TEXT("Rubezh.Hurt"), TEXT("Ранить себя: Rubezh.Hurt <урон>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HurtPlayer));

	FAutoConsoleCommandWithWorldAndArgs EquipCommand(
		TEXT("Rubezh.Equip"), TEXT("Надеть предмет: Rubezh.Equip <ItemId>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&EquipItem));

	FAutoConsoleCommandWithWorldAndArgs UnequipCommand(
		TEXT("Rubezh.Unequip"), TEXT("Снять снаряжение: Rubezh.Unequip head|armor|top|bottom|backpack"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&UnequipItem));

	FAutoConsoleCommandWithWorldAndArgs ShootCommand(
		TEXT("Rubezh.Shoot"), TEXT("Попадание в себя через расчёт урона Lyra: Rubezh.Shoot <урон> [head|torso|arms|legs]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ShootSelf));

	FAutoConsoleCommandWithWorldAndArgs InventoryCommand(
		TEXT("Rubezh.Inventory"), TEXT("Показать содержимое рюкзака и вес"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PrintInventory));
}

#endif // !UE_BUILD_SHIPPING
