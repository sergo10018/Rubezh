// РУБЕЖ: Земля после разлома
// Отладочные консольные команды для плейтеста. В сборку Shipping не попадают.

#include "Animals/RubezhAnimal.h"
#include "Animals/RubezhAnimalSpawner.h"
#include "EngineUtils.h"
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
#include "Items/RubezhItemCatalog.h"
#include "Items/RubezhLootContainer.h"
#include "Items/RubezhPickupRules.h"
#include "EngineUtils.h"
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
			[Inventory](FName Id) { return Inventory->FindDefinition(Id); }, URubezhInventoryComponent::GetPocketSlots());
		Inventory->ServerEquipEntry(Index);
		UE_LOG(LogRubezh, Display, TEXT("Rubezh.Equip %s: %s"), *Args[0],
			Result == Rubezh::Equipment::EResult::Ok ? TEXT("надето") : Rubezh::Equipment::Describe(Result));
	}

	/** Выпустить животных в 15 м перед игроком. */
	void SpawnAnimals(const TArray<FString>& Args, UWorld* World)
	{
		const APawn* Pawn = FindLocalPawnOnServer(World);
		if (!Pawn || Args.Num() < 1)
		{
			UE_LOG(LogRubezh, Display, TEXT("Rubezh.Animal dog|wolf|boar|bear [сколько] — выпустить животных перед собой (только на сервере)"));
			return;
		}
		const int32 Count = Args.Num() > 1 ? FCString::Atoi(*Args[1]) : 1;
		ARubezhAnimalSpawner::SpawnAt(*World, Pawn->GetActorLocation() + Pawn->GetActorForwardVector() * 1500.0f, FName(*Args[0]), Count);
	}

	/** Попадание по ближайшему зверю от имени игрока, через расчёт урона Lyra: проверить здоровье, гибель и бегство стаи. */
	void HitNearestAnimal(const TArray<FString>& Args, UWorld* World)
	{
		APawn* Pawn = FindLocalPawnOnServer(World);
		UAbilitySystemComponent* Source = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn);
		const TSubclassOf<UGameplayEffect> DamageEffect = LoadClass<UGameplayEffect>(nullptr,
			TEXT("/Game/GameplayEffects/Damage/GE_Damage_Basic_SetByCaller.GE_Damage_Basic_SetByCaller_C"));
		ARubezhAnimal* Nearest = nullptr;
		for (TActorIterator<ARubezhAnimal> It(World); It && Pawn; ++It)
		{
			if (It->IsAlive() && (!Nearest || It->GetDistanceTo(Pawn) < Nearest->GetDistanceTo(Pawn)))
			{
				Nearest = *It;
			}
		}
		if (!Source || !DamageEffect || !Nearest || Args.Num() < 1)
		{
			UE_LOG(LogRubezh, Display, TEXT("Rubezh.HitAnimal <урон> — попасть по ближайшему зверю"));
			return;
		}

		FGameplayEffectContextHandle Context = Source->MakeEffectContext();
		FHitResult Hit;
		Hit.HitObjectHandle = FActorInstanceHandle(Nearest);
		Hit.ImpactPoint = Nearest->GetActorLocation();
		Hit.Location = Hit.ImpactPoint;
		Context.AddHitResult(Hit, true);
		const FGameplayEffectSpecHandle Spec = Source->MakeOutgoingSpec(DamageEffect, 1.0f, Context);
		Spec.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(TEXT("SetByCaller.Damage")), FCString::Atof(*Args[0]));
		const float Before = Nearest->GetHealthFraction();
		Source->ApplyGameplayEffectSpecToTarget(*Spec.Data, Nearest->GetAbilitySystemComponent());
		UE_LOG(LogRubezh, Display, TEXT("Rubezh.HitAnimal: %s, здоровье %.0f %% → %.0f %%, состояние %s"),
			*Nearest->GetAnimalId().ToString(), Before * 100.0f, Nearest->GetHealthFraction() * 100.0f, *UEnum::GetValueAsString(Nearest->GetState()));
	}

	/** Что сейчас делают звери: вид, состояние, расстояние до игрока. */
	void ListAnimals(const TArray<FString>& Args, UWorld* World)
	{
		const APawn* Pawn = FindLocalPawnOnServer(World);
		for (TActorIterator<ARubezhAnimal> It(World); It && Pawn; ++It)
		{
			UE_LOG(LogRubezh, Display, TEXT("%s %s: %s, %.0f м, здоровье %.0f %%"), *It->GetName(), *It->GetAnimalId().ToString(),
				*UEnum::GetValueAsString(It->GetState()), It->GetDistanceTo(Pawn) * 0.01f, It->GetHealthFraction() * 100.0f);
		}
	}

	ERubezhEquipSlot ParseEquipSlot(const FString& Name)
	{
		return Name == TEXT("head") ? ERubezhEquipSlot::Head :
			Name == TEXT("armor") ? ERubezhEquipSlot::Armor :
			Name == TEXT("top") ? ERubezhEquipSlot::Top :
			Name == TEXT("bottom") ? ERubezhEquipSlot::Bottom :
			Name == TEXT("backpack") ? ERubezhEquipSlot::Backpack : ERubezhEquipSlot::None;
	}

	/** Задать прочность надетого — проверить, что защита падает вместе с ней. */
	void WearGear(const TArray<FString>& Args, UWorld* World)
	{
		URubezhInventoryComponent* Inventory = (World && World->GetNetMode() != NM_Client) ? FindLocalInventory(World) : nullptr;
		const ERubezhEquipSlot Slot = Args.Num() > 0 ? ParseEquipSlot(Args[0].ToLower()) : ERubezhEquipSlot::None;
		if (!Inventory || Slot == ERubezhEquipSlot::None || Args.Num() < 2)
		{
			UE_LOG(LogRubezh, Display, TEXT("Rubezh.GearWear head|armor|top|bottom <проценты> — прочность надетого (только на сервере)"));
			return;
		}
		Inventory->SetEquippedDurability(Slot, FCString::Atof(*Args[1]));
	}

	void RepairGear(const TArray<FString>& Args, UWorld* World)
	{
		URubezhInventoryComponent* Inventory = FindLocalInventory(World);
		const ERubezhEquipSlot Slot = Args.Num() > 0 ? ParseEquipSlot(Args[0].ToLower()) : ERubezhEquipSlot::None;
		if (Inventory && Slot != ERubezhEquipSlot::None)
		{
			Inventory->ServerRepairEquipped(Slot);
		}
	}

	void UnequipItem(const TArray<FString>& Args, UWorld* World)
	{
		URubezhInventoryComponent* Inventory = (World && World->GetNetMode() != NM_Client) ? FindLocalInventory(World) : nullptr;
		const FString Name = Args.Num() > 0 ? Args[0].ToLower() : FString();
		const ERubezhEquipSlot Slot = ParseEquipSlot(Name);
		if (!Inventory || Slot == ERubezhEquipSlot::None)
		{
			UE_LOG(LogRubezh, Display, TEXT("Rubezh.Unequip head|armor|top|bottom|backpack — снять в рюкзак"));
			return;
		}

		FRubezhInventoryGrid Grid = Inventory->GetGrid();
		Rubezh::Equipment::FEquipped Equipped = Inventory->GetEquippedItems();
		const Rubezh::Equipment::EResult Result = Rubezh::Equipment::Unequip(Grid, Equipped, Slot, {},
			[Inventory](FName Id) { return Inventory->FindDefinition(Id); }, URubezhInventoryComponent::GetPocketSlots());
		Inventory->ServerUnequip(Slot, INDEX_NONE);
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
			UE_LOG(LogRubezh, Display, TEXT("  [%s] %s, прочность %.1f"), Rubezh::Equipment::GetSlotName(Slot), *Inventory->GetEquipped(Slot).ToString(), Inventory->GetEquippedDurability(Slot));
		}
		const Rubezh::Weapons::FWeaponSlots& Weapons = Inventory->GetWeaponSlots();
		for (int32 Index = 0; Index < Weapons.Num(); ++Index)
		{
			UE_LOG(LogRubezh, Display, TEXT("  [%s] %s, патронов %d, магазин %d, прочность %.2f"), Rubezh::Weapons::GetSlotName(Index),
				*Weapons[Index].ItemId.ToString(), Weapons[Index].Rounds, Weapons[Index].bHasMagazine ? 1 : 0, Weapons[Index].Durability);
		}
		const FRubezhInventoryGrid& Grid = Inventory->GetGrid();
		UE_LOG(LogRubezh, Display, TEXT("Билеты: %d"), Inventory->GetMoney());
		UE_LOG(LogRubezh, Display, TEXT("Рюкзак: занято %d из %d ячеек, вес %.2f кг"),
			Grid.GetEntries().Num(), Grid.GetSlotCount(), Inventory->GetTotalWeight());
		for (const FRubezhInventoryEntry& Entry : Grid.GetEntries())
		{
			UE_LOG(LogRubezh, Display, TEXT("  [%d] %s ×%d, патронов %d"), Entry.Slot, *Entry.ItemId.ToString(), Entry.Count, Entry.Rounds);
		}

		if (const ARubezhLootContainer* Container = Inventory->GetLootContainer())
		{
			const FRubezhInventoryGrid& Loot = Inventory->GetLootGrid();
			UE_LOG(LogRubezh, Display, TEXT("Обыск «%s»: занято %d из %d ячеек%s"), *Container->GetDisplayName().ToString(),
				Loot.GetEntries().Num(), Loot.GetSlotCount(), Container->GetBagItemId().IsNone() ? TEXT("") : TEXT(" (брошенный рюкзак)"));
			for (const FRubezhInventoryEntry& Entry : Loot.GetEntries())
			{
				UE_LOG(LogRubezh, Display, TEXT("  [%d] %s ×%d"), Entry.Slot, *Entry.ItemId.ToString(), Entry.Count);
			}
		}
	}

	/** Выдать заряженное оружие, два полных запасных магазина (или патроны к дробовику). */
	void ArmPlayer(const TArray<FString>& Args, UWorld* World)
	{
		URubezhInventoryComponent* Inventory = FindLocalInventory(World);
		const FName ItemId = Args.Num() > 0 ? FName(*Args[0]) : NAME_None;
		const FRubezhWeaponDefinition* Weapon = Rubezh::Items::FindWeapon(ItemId);
		if (!Inventory || !Weapon || !World || World->GetNetMode() == NM_Client)
		{
			UE_LOG(LogRubezh, Display, TEXT("Rubezh.Arm pistol_9x18|rifle_762x39|shotgun_12|rifle_762x54r — заряженное оружие и запас (только на сервере)"));
			return;
		}
		if (!Inventory->GiveLoadedWeapon(ItemId))
		{
			UE_LOG(LogRubezh, Warning, TEXT("Rubezh.Arm: нет свободного подходящего слота"));
			return;
		}
		if (const FRubezhItemDefinition* Magazine = Inventory->FindDefinition(Weapon->MagazineItem))
		{
			for (int32 Index = 0; Index < 2; ++Index)
			{
				FRubezhInventoryEntry Spare;
				Spare.ItemId = Weapon->MagazineItem;
				Spare.Count = 1;
				Spare.Rounds = Magazine->MagazineCapacity;
				Inventory->AddEntry(Spare);
			}
		}
		else
		{
			Inventory->AddItem(Weapon->AmmoItem, Weapon->TubeCapacity * 2);
		}
	}

	/** Задать прочность оружия в руках — чтобы проверить осечки. */
	void WearWeapon(const TArray<FString>& Args, UWorld* World)
	{
		URubezhInventoryComponent* Inventory = FindLocalInventory(World);
		if (!Inventory || Args.Num() < 1 || !World || World->GetNetMode() == NM_Client)
		{
			UE_LOG(LogRubezh, Display, TEXT("Rubezh.WeaponWear <проценты> — прочность оружия в руках (только на сервере)"));
			return;
		}
		Inventory->SetWeaponDurability(Inventory->GetActiveWeaponSlot(), FCString::Atof(*Args[0]));
	}

	/** Набить или разрядить магазин в ячейке, бросить оружие — те же запросы, что шлёт окно рюкзака. */
	void LoadMagazine(const TArray<FString>& Args, UWorld* World)
	{
		URubezhInventoryComponent* Inventory = FindLocalInventory(World);
		if (Inventory && Args.Num() > 0)
		{
			Inventory->ServerStartLoadingMagazine(Inventory->GetGrid().FindEntryAt(FCString::Atoi(*Args[0])));
		}
	}

	void UnloadMagazine(const TArray<FString>& Args, UWorld* World)
	{
		URubezhInventoryComponent* Inventory = FindLocalInventory(World);
		if (Inventory && Args.Num() > 0)
		{
			Inventory->ServerUnloadMagazine(Inventory->GetGrid().FindEntryAt(FCString::Atoi(*Args[0])));
		}
	}

	void DropWeapon(const TArray<FString>& Args, UWorld* World)
	{
		URubezhInventoryComponent* Inventory = FindLocalInventory(World);
		if (Inventory && Args.Num() > 0)
		{
			Inventory->ServerDropWeapon(FCString::Atoi(*Args[0]));
		}
	}

	/** Деньги и сделки — те же запросы, что шлёт окно торговца. */
	void SetMoney(const TArray<FString>& Args, UWorld* World)
	{
		URubezhInventoryComponent* Inventory = (World && World->GetNetMode() != NM_Client) ? FindLocalInventory(World) : nullptr;
		if (!Inventory || Args.Num() < 1)
		{
			UE_LOG(LogRubezh, Display, TEXT("Rubezh.Money <билеты> — задать счёт (только на сервере)"));
			return;
		}
		Inventory->SetMoney(FCString::Atoi(*Args[0]));
		UE_LOG(LogRubezh, Display, TEXT("Rubezh.Money: на счёте %d"), Inventory->GetMoney());
	}

	void BuyItem(const TArray<FString>& Args, UWorld* World)
	{
		URubezhInventoryComponent* Inventory = FindLocalInventory(World);
		if (Inventory && Args.Num() > 0)
		{
			Inventory->ServerBuyItem(FName(*Args[0]), Args.Num() > 1 ? FCString::Atoi(*Args[1]) : 1);
		}
	}

	void SellCell(const TArray<FString>& Args, UWorld* World)
	{
		URubezhInventoryComponent* Inventory = FindLocalInventory(World);
		const int32 Cell = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : INDEX_NONE;
		const int32 Index = Inventory ? Inventory->GetGrid().FindEntryAt(Cell) : INDEX_NONE;
		if (Index != INDEX_NONE)
		{
			Inventory->ServerSellEntry(Cell, Inventory->GetGrid().GetEntries()[Index].ItemId);
		}
	}

	void RepairSlot(const TArray<FString>& Args, UWorld* World)
	{
		URubezhInventoryComponent* Inventory = FindLocalInventory(World);
		if (Inventory && Args.Num() > 0)
		{
			Inventory->ServerRepairWeapon(FCString::Atoi(*Args[0]));
		}
	}

	/** Применить предмет — тот же запрос, что двойной клик в рюкзаке. */
	void UseInventoryItem(const TArray<FString>& Args, UWorld* World)
	{
		const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		URubezhInjuryComponent* Injuries = Controller ? URubezhInjuryComponent::FindForActor(Controller->GetPawn()) : nullptr;
		if (Injuries && Args.Num() > 0)
		{
			Injuries->UseItem(FName(*Args[0]));
		}
	}

	/** Поставить перед собой ящик с лутом. */
	void SpawnCrate(const TArray<FString>& Args, UWorld* World)
	{
		APawn* Pawn = FindLocalPawnOnServer(World);
		if (!Pawn || Args.Num() < 1)
		{
			UE_LOG(LogRubezh, Display, TEXT("Rubezh.Crate <military|medical|civil> [ячеек] — ящик с лутом перед собой"));
			return;
		}

		const int32 Slots = Args.Num() > 1 ? FCString::Atoi(*Args[1]) : 12;
		const FVector Forward = Pawn->GetActorForwardVector().GetSafeNormal2D();
		const FVector Location = ARubezhWorldItem::FindGround(*World, Pawn->GetActorLocation() + Forward * 120.0f, Pawn);
		const ARubezhLootContainer* Crate = ARubezhLootContainer::SpawnCrate(*World,
			FTransform(FRotator(0.0f, Pawn->GetActorRotation().Yaw, 0.0f), Location), FName(*Args[0]), Slots);
		UE_LOG(LogRubezh, Display, TEXT("Rubezh.Crate %s: %s"), *Args[0], Crate ? TEXT("поставлен") : TEXT("не удалось"));
	}

	/** Обыскать ближайшее в досягаемости (как E, но без прицеливания). */
	void SearchNearest(const TArray<FString>& Args, UWorld* World)
	{
		APawn* Pawn = FindLocalPawnOnServer(World);
		URubezhInventoryComponent* Inventory = URubezhInventoryComponent::FindForActor(Pawn);
		ARubezhLootContainer* Nearest = nullptr;
		for (TActorIterator<ARubezhLootContainer> It(World); Pawn && It; ++It)
		{
			if (Rubezh::Pickup::IsWithinReach(Pawn->GetActorLocation(), It->GetFocusLocation(), false)
				&& (!Nearest || FVector::Dist(Pawn->GetActorLocation(), It->GetActorLocation()) < FVector::Dist(Pawn->GetActorLocation(), Nearest->GetActorLocation())))
			{
				Nearest = *It;
			}
		}
		if (!Inventory || !Nearest)
		{
			UE_LOG(LogRubezh, Display, TEXT("Rubezh.Search: рядом нечего обыскивать"));
			return;
		}
		Inventory->ServerStartSearch(Nearest);
		UE_LOG(LogRubezh, Display, TEXT("Rubezh.Search: обыск «%s», %.1f с"), *Nearest->GetDisplayName().ToString(), Inventory->GetSearchSecondsLeft());
	}

	void TakeAllLoot(const TArray<FString>& Args, UWorld* World)
	{
		if (URubezhInventoryComponent* Inventory = URubezhInventoryComponent::FindForActor(FindLocalPawnOnServer(World)))
		{
			Inventory->ServerTakeAllLoot();
			PrintInventory(Args, World);
		}
	}

	void WearLootBag(const TArray<FString>& Args, UWorld* World)
	{
		if (URubezhInventoryComponent* Inventory = URubezhInventoryComponent::FindForActor(FindLocalPawnOnServer(World)))
		{
			Inventory->ServerWearLootBag();
			PrintInventory(Args, World);
		}
	}

	void DropBackpack(const TArray<FString>& Args, UWorld* World)
	{
		if (URubezhInventoryComponent* Inventory = URubezhInventoryComponent::FindForActor(FindLocalPawnOnServer(World)))
		{
			Inventory->ServerDropEquipped(ERubezhEquipSlot::Backpack);
			PrintInventory(Args, World);
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

	FAutoConsoleCommandWithWorldAndArgs ArmCommand(
		TEXT("Rubezh.Arm"), TEXT("Заряженное оружие и запас: Rubezh.Arm <ItemId>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ArmPlayer));

	FAutoConsoleCommandWithWorldAndArgs WeaponWearCommand(
		TEXT("Rubezh.WeaponWear"), TEXT("Прочность оружия в руках: Rubezh.WeaponWear <проценты>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&WearWeapon));

	FAutoConsoleCommandWithWorldAndArgs LoadMagazineCommand(
		TEXT("Rubezh.LoadMag"), TEXT("Набить магазин патронами из рюкзака: Rubezh.LoadMag <ячейка>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&LoadMagazine));

	FAutoConsoleCommandWithWorldAndArgs UnloadMagazineCommand(
		TEXT("Rubezh.UnloadMag"), TEXT("Разрядить магазин: Rubezh.UnloadMag <ячейка>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&UnloadMagazine));

	FAutoConsoleCommandWithWorldAndArgs DropWeaponCommand(
		TEXT("Rubezh.DropWeapon"), TEXT("Бросить оружие из слота: Rubezh.DropWeapon <0|1|2>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&DropWeapon));

	FAutoConsoleCommandWithWorldAndArgs CrateCommand(
		TEXT("Rubezh.Crate"), TEXT("Ящик с лутом перед собой: Rubezh.Crate <military|medical|civil> [ячеек]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SpawnCrate));

	FAutoConsoleCommandWithWorldAndArgs SearchCommand(
		TEXT("Rubezh.Search"), TEXT("Обыскать ближайший ящик, тело или рюкзак в досягаемости"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SearchNearest));

	FAutoConsoleCommandWithWorldAndArgs TakeAllCommand(
		TEXT("Rubezh.TakeAll"), TEXT("Забрать всё из обыскиваемого"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TakeAllLoot));

	FAutoConsoleCommandWithWorldAndArgs WearBagCommand(
		TEXT("Rubezh.WearBag"), TEXT("Надеть обыскиваемый брошенный рюкзак"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&WearLootBag));

	FAutoConsoleCommandWithWorldAndArgs DropBagCommand(
		TEXT("Rubezh.DropBag"), TEXT("Сбросить рюкзак вместе с вещами"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&DropBackpack));

	FAutoConsoleCommandWithWorldAndArgs MoneyCommand(
		TEXT("Rubezh.Money"), TEXT("Задать счёт, билеты"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SetMoney));

	FAutoConsoleCommandWithWorldAndArgs BuyCommand(
		TEXT("Rubezh.Buy"), TEXT("Купить у открытого торговца: <id> [штук]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&BuyItem));

	FAutoConsoleCommandWithWorldAndArgs SellCommand(
		TEXT("Rubezh.Sell"), TEXT("Продать открытому торговцу стопку из ячейки: <ячейка>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SellCell));

	FAutoConsoleCommandWithWorldAndArgs RepairCommand(
		TEXT("Rubezh.Repair"), TEXT("Починить у оружейника оружие в слоте: <слот 0-2>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&RepairSlot));

	FAutoConsoleCommandWithWorldAndArgs AnimalCommand(
		TEXT("Rubezh.Animal"), TEXT("Выпустить животных перед собой: dog|wolf|boar|bear [сколько]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SpawnAnimals));

	FAutoConsoleCommandWithWorldAndArgs HitAnimalCommand(
		TEXT("Rubezh.HitAnimal"), TEXT("Попасть по ближайшему зверю: <урон>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HitNearestAnimal));

	FAutoConsoleCommandWithWorldAndArgs AnimalsCommand(
		TEXT("Rubezh.Animals"), TEXT("Показать зверей: состояние и расстояние"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ListAnimals));

	FAutoConsoleCommandWithWorldAndArgs GearWearCommand(
		TEXT("Rubezh.GearWear"), TEXT("Прочность надетого: head|armor|top|bottom <проценты>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&WearGear));

	FAutoConsoleCommandWithWorldAndArgs RepairGearCommand(
		TEXT("Rubezh.RepairGear"), TEXT("Починить у бронника надетое: head|armor|top|bottom"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&RepairGear));

	FAutoConsoleCommandWithWorldAndArgs UseCommand(
		TEXT("Rubezh.Use"), TEXT("Применить предмет из рюкзака: Rubezh.Use <ItemId>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&UseInventoryItem));

	FAutoConsoleCommandWithWorldAndArgs InventoryCommand(
		TEXT("Rubezh.Inventory"), TEXT("Показать содержимое рюкзака и вес"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PrintInventory));
}

#endif // !UE_BUILD_SHIPPING
