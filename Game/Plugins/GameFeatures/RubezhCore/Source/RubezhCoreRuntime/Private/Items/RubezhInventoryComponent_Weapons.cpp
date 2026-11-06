// РУБЕЖ: Земля после разлома
// Оружие: слоты, связь с оружием Lyra, магазины, перезарядка, износ, осечки, отдача.
//
// Как это связано с Lyra. Стрельбу, анимации и счётчик патронов ведёт Lyra: для оружия в слоте создаётся её предмет
// (ID_*) в быстром доступе (клавиши 1, 2, 3). Главная запись — WeaponSlots; на предмете Lyra лежит копия чисел:
//   MagazineAmmo — патроны в оружии, MagazineSize — вместимость, SpareAmmo — запас в рюкзаке.
// Перезарядка Lyra (GA_Weapon_ReloadMagazine) сама считает патроны, ничего не зная о магазинах, поэтому после неё,
// на следующем кадре, сервер меняет магазин по нашим правилам и переписывает копию.

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Character/LyraHealthComponent.h"
#include "Equipment/LyraEquipmentManagerComponent.h"
#include "Equipment/LyraQuickBarComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Inventory/LyraInventoryItemInstance.h"
#include "Inventory/LyraInventoryManagerComponent.h"
#include "Items/RubezhInteractionComponent.h"
#include "Items/RubezhInventoryComponent.h"
#include "Items/RubezhItemCatalog.h"
#include "Items/RubezhWorldItem.h"
#include "RubezhCoreRuntimeModule.h"
#include "RubezhGameplayTags.h"
#include "RubezhSettings.h"
#include "TimerManager.h"

namespace
{
	constexpr float WeaponInstanceRetrySeconds = 0.25f;
	constexpr int32 MaxWeaponInstanceRetries = 80;	// 20 секунд

	FGameplayTag MagazineAmmoTag() { return FGameplayTag::RequestGameplayTag(TEXT("Lyra.ShooterGame.Weapon.MagazineAmmo")); }
	FGameplayTag MagazineSizeTag() { return FGameplayTag::RequestGameplayTag(TEXT("Lyra.ShooterGame.Weapon.MagazineSize")); }
	FGameplayTag SpareAmmoTag() { return FGameplayTag::RequestGameplayTag(TEXT("Lyra.ShooterGame.Weapon.SpareAmmo")); }
	FGameplayTag ReloadDoneTag() { return FGameplayTag::RequestGameplayTag(TEXT("GameplayEvent.ReloadDone")); }

	void SetStatTag(ULyraInventoryItemInstance& Item, const FGameplayTag& Tag, int32 Value)
	{
		const int32 Current = Item.GetStatTagStackCount(Tag);
		if (Value > Current)
		{
			Item.AddStatTagStack(Tag, Value - Current);
		}
		else if (Value < Current)
		{
			Item.RemoveStatTagStack(Tag, Current - Value);
		}
	}
}

// --- Запросы игрока ---

bool URubezhInventoryComponent::IsOwnerDead() const
{
	const ULyraHealthComponent* Health = BoundHealth.Get();
	return Health && Health->IsDeadOrDying();
}

void URubezhInventoryComponent::ServerEquipWeapon_Implementation(int32 EntryIndex, int32 WeaponSlot)
{
	if (IsOwnerDead())
	{
		return;
	}

	const TOptional<int32> Target = WeaponSlot == INDEX_NONE ? TOptional<int32>() : TOptional<int32>(WeaponSlot);
	const Rubezh::Weapons::EResult Result = Rubezh::Weapons::Equip(Grid, WeaponSlots, EntryIndex, Target, [](FName Id) { return Rubezh::Items::FindWeapon(Id); });
	if (Result == Rubezh::Weapons::EResult::Ok)
	{
		HandleWeaponsChanged();
	}
}

void URubezhInventoryComponent::ServerUnequipWeapon_Implementation(int32 WeaponSlot, int32 Cell)
{
	if (IsOwnerDead())
	{
		return;
	}

	const TOptional<int32> Target = Cell == INDEX_NONE ? TOptional<int32>() : TOptional<int32>(Cell);
	if (Rubezh::Weapons::Unequip(Grid, WeaponSlots, WeaponSlot, Target) == Rubezh::Weapons::EResult::Ok)
	{
		HandleWeaponsChanged();
	}
}

void URubezhInventoryComponent::ServerDropWeapon_Implementation(int32 WeaponSlot)
{
	APawn* Pawn = GetOwnerPawn();
	if (!Pawn || IsOwnerDead() || !WeaponSlots.IsValidIndex(WeaponSlot) || WeaponSlots[WeaponSlot].ItemId.IsNone())
	{
		return;
	}

	// Сначала кладём на землю: если положить не вышло, оружие останется в слоте.
	if (ARubezhWorldItem::SpawnEntryInFrontOf(*Pawn, WeaponSlots[WeaponSlot]))
	{
		WeaponSlots[WeaponSlot] = FRubezhInventoryEntry();
		HandleWeaponsChanged();
	}
}

void URubezhInventoryComponent::ServerStartLoadingMagazine_Implementation(int32 EntryIndex)
{
	auto Lookup = [this](FName Id) { return FindDefinition(Id); };
	if (!Rubezh::Weapons::CanLoadRound(Grid, EntryIndex, Lookup))
	{
		return;
	}

	LoadingMagazineCell = Grid.GetEntries()[EntryIndex].Slot;
	GetWorld()->GetTimerManager().SetTimer(MagazineLoadTimer, this, &ThisClass::LoadNextRound, Rubezh::Weapons::SecondsPerLoadedRound, true);
}

void URubezhInventoryComponent::ServerStopLoadingMagazine_Implementation()
{
	LoadingMagazineCell = INDEX_NONE;
	GetWorld()->GetTimerManager().ClearTimer(MagazineLoadTimer);
}

void URubezhInventoryComponent::LoadNextRound()
{
	// Магазин ищем по ячейке: если его переложили или выбросили, набивка просто заканчивается.
	auto Lookup = [this](FName Id) { return FindDefinition(Id); };
	if (!Rubezh::Weapons::LoadRound(Grid, Grid.FindEntryAt(LoadingMagazineCell), Lookup))
	{
		ServerStopLoadingMagazine_Implementation();
		return;
	}
	HandleGridChanged();
}

void URubezhInventoryComponent::ServerUnloadMagazine_Implementation(int32 EntryIndex)
{
	if (Rubezh::Weapons::UnloadMagazine(Grid, EntryIndex, [this](FName Id) { return FindDefinition(Id); }) > 0)
	{
		HandleGridChanged();
	}
}

// --- Оружие Lyra ---

ULyraQuickBarComponent* URubezhInventoryComponent::FindQuickBar() const
{
	const APlayerState* PlayerState = Cast<APlayerState>(GetOwner());
	const AController* Controller = PlayerState ? PlayerState->GetOwningController() : nullptr;
	return Controller ? Controller->FindComponentByClass<ULyraQuickBarComponent>() : nullptr;
}

int32 URubezhInventoryComponent::GetActiveWeaponSlot() const
{
	const ULyraQuickBarComponent* QuickBar = FindQuickBar();
	return QuickBar ? QuickBar->GetActiveSlotIndex() : INDEX_NONE;
}

void URubezhInventoryComponent::ApplyStartingWeapons()
{
	const URubezhSettings* Settings = GetDefault<URubezhSettings>();
	for (const FName& ItemId : Settings->StartingWeapons)
	{
		if (!GiveLoadedWeapon(ItemId))
		{
			UE_LOG(LogRubezh, Warning, TEXT("Rubezh: стартовое оружие '%s' не удалось выдать"), *ItemId.ToString());
			continue;
		}

		const FRubezhWeaponDefinition* Weapon = Rubezh::Items::FindWeapon(ItemId);
		const FRubezhItemDefinition* Magazine = Weapon ? FindDefinition(Weapon->MagazineItem) : nullptr;
		for (int32 Index = 0; Magazine && Index < Settings->StartingSpareMagazines; ++Index)
		{
			FRubezhInventoryEntry Spare;
			Spare.ItemId = Weapon->MagazineItem;
			Spare.Count = 1;
			Spare.Rounds = Magazine->MagazineCapacity;
			Grid.AddEntry(Spare, *Magazine);
		}
	}
	HandleGridChanged();
}

bool URubezhInventoryComponent::GiveLoadedWeapon(FName ItemId)
{
	const FRubezhWeaponDefinition* Weapon = Rubezh::Items::FindWeapon(ItemId);
	if (!Weapon)
	{
		return false;
	}

	FRubezhInventoryEntry Entry = Rubezh::Weapons::MakeLoaded(ItemId, *Weapon, [this](FName Id) { return FindDefinition(Id); });
	for (int32 Index = 0; Index < Rubezh::Weapons::SlotCount; ++Index)
	{
		if (WeaponSlots[Index].ItemId.IsNone() && Rubezh::Weapons::SlotAccepts(Weapon->WeaponClass, Index))
		{
			WeaponSlots[Index] = MoveTemp(Entry);
			HandleWeaponsChanged();
			return true;
		}
	}
	return false;
}

void URubezhInventoryComponent::SetWeaponDurability(int32 WeaponSlot, float Durability)
{
	if (WeaponSlots.IsValidIndex(WeaponSlot) && !WeaponSlots[WeaponSlot].ItemId.IsNone())
	{
		WeaponSlots[WeaponSlot].Durability = FMath::Clamp(Durability, 0.0f, Rubezh::Weapons::MaxDurability);
		RollNextShot(WeaponSlots[WeaponSlot]);
		HandleGridChanged();
	}
}

void URubezhInventoryComponent::HandleWeaponsChanged()
{
	HandleGridChanged();
	if (GetOwnerPawn())
	{
		RebuildWeaponInstances();
	}
}

void URubezhInventoryComponent::RebuildWeaponInstances()
{
	ActiveWeaponSlotBeforeRebuild = GetActiveWeaponSlot();
	ClearWeaponInstances();
	WeaponInstanceRetries = 0;
	TryCreateWeaponInstances();
}

void URubezhInventoryComponent::TryCreateWeaponInstances()
{
	// Персонаж, его способности и быстрый доступ появляются не одновременно — ждём всех.
	const APawn* Pawn = GetOwnerPawn();
	if (!Pawn || IsOwnerDead())
	{
		return;	// оружие выдастся новому персонажу после возрождения
	}

	ULyraQuickBarComponent* QuickBar = FindQuickBar();
	const UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
	const bool bReady = QuickBar && ASC && ASC->GetAvatarActor() == Pawn && Pawn->GetController()
		&& Pawn->FindComponentByClass<ULyraEquipmentManagerComponent>();
	if (!bReady)
	{
		if (++WeaponInstanceRetries > MaxWeaponInstanceRetries)
		{
			UE_LOG(LogRubezh, Warning, TEXT("%s: персонаж не готов принять оружие, больше не ждём"), *GetNameSafe(GetOwner()));
			return;
		}
		GetWorld()->GetTimerManager().SetTimer(WeaponInstanceTimer, this, &ThisClass::TryCreateWeaponInstances, WeaponInstanceRetrySeconds, false);
		return;
	}

	int32 FirstOccupied = INDEX_NONE;
	for (int32 Index = 0; Index < Rubezh::Weapons::SlotCount; ++Index)
	{
		if (!WeaponSlots[Index].ItemId.IsNone())
		{
			CreateWeaponInstance(Index);
			FirstOccupied = FirstOccupied == INDEX_NONE ? Index : FirstOccupied;
		}
	}
	SyncWeaponTags();

	const bool bKeepActive = WeaponSlots.IsValidIndex(ActiveWeaponSlotBeforeRebuild) && !WeaponSlots[ActiveWeaponSlotBeforeRebuild].ItemId.IsNone();
	const int32 NewActive = bKeepActive ? ActiveWeaponSlotBeforeRebuild : FirstOccupied;
	if (NewActive != INDEX_NONE)
	{
		QuickBar->SetActiveSlotIndex(NewActive);
	}
}

void URubezhInventoryComponent::CreateWeaponInstance(int32 WeaponSlot)
{
	const FRubezhInventoryEntry& Entry = WeaponSlots[WeaponSlot];
	const FRubezhWeaponDefinition* Weapon = Rubezh::Items::FindWeapon(Entry.ItemId);
	const APlayerState* PlayerState = Cast<APlayerState>(GetOwner());
	const AController* Controller = PlayerState ? PlayerState->GetOwningController() : nullptr;
	ULyraInventoryManagerComponent* Manager = Controller ? Controller->FindComponentByClass<ULyraInventoryManagerComponent>() : nullptr;
	UClass* ItemClass = Weapon ? Weapon->LyraItem.LoadSynchronous() : nullptr;
	if (!Manager || !ItemClass)
	{
		UE_LOG(LogRubezh, Error, TEXT("Rubezh: не удалось создать оружие '%s' (нет инвентаря Lyra или ID_* из data/weapons.csv)"), *Entry.ItemId.ToString());
		return;
	}

	ULyraInventoryItemInstance* Item = Manager->AddItemDefinition(ItemClass, 1);
	if (!Item)
	{
		return;
	}

	SetStatTag(*Item, RubezhGameplayTags::Weapon_Damage, Weapon->Damage);
	SetStatTag(*Item, RubezhGameplayTags::Weapon_EffectiveRange, FMath::RoundToInt(Weapon->EffectiveRangeM));
	WeaponInstances[WeaponSlot] = Item;
	FindQuickBar()->AddItemToSlot(WeaponSlot, Item);
}

void URubezhInventoryComponent::ClearWeaponInstances()
{
	GetWorld()->GetTimerManager().ClearTimer(WeaponInstanceTimer);

	ULyraQuickBarComponent* QuickBar = FindQuickBar();
	const APlayerState* PlayerState = Cast<APlayerState>(GetOwner());
	const AController* Controller = PlayerState ? PlayerState->GetOwningController() : nullptr;
	ULyraInventoryManagerComponent* Manager = Controller ? Controller->FindComponentByClass<ULyraInventoryManagerComponent>() : nullptr;
	for (int32 Index = 0; Index < WeaponInstances.Num(); ++Index)
	{
		if (QuickBar)
		{
			QuickBar->RemoveItemFromSlot(Index);
		}
		if (Manager && WeaponInstances[Index])
		{
			Manager->RemoveItemInstance(WeaponInstances[Index]);
		}
		WeaponInstances[Index] = nullptr;
	}
}

void URubezhInventoryComponent::SyncWeaponTags() const
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	auto Lookup = [this](FName Id) { return FindDefinition(Id); };
	for (int32 Index = 0; Index < WeaponInstances.Num(); ++Index)
	{
		ULyraInventoryItemInstance* Item = WeaponInstances[Index];
		const FRubezhWeaponDefinition* Weapon = Item ? Rubezh::Items::FindWeapon(WeaponSlots[Index].ItemId) : nullptr;
		if (Weapon)
		{
			SetStatTag(*Item, MagazineSizeTag(), Rubezh::Weapons::GetCapacity(*Weapon, Lookup));
			SetStatTag(*Item, MagazineAmmoTag(), WeaponSlots[Index].Rounds);
			SetStatTag(*Item, SpareAmmoTag(), Rubezh::Weapons::CountSpareRounds(Grid, *Weapon));
			SetStatTag(*Item, RubezhGameplayTags::Weapon_NextShotMisfires, WeaponSlots[Index].bNextShotMisfires ? 1 : 0);
		}
	}
}

// --- Перезарядка ---

void URubezhInventoryComponent::BindReloadEvent()
{
	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
	{
		ReloadEventHandle = ASC->GenericGameplayEventCallbacks.FindOrAdd(ReloadDoneTag()).AddUObject(this, &ThisClass::HandleReloadDone);
	}
}

void URubezhInventoryComponent::UnbindReloadEvent()
{
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
	if (ASC && ReloadEventHandle.IsValid())
	{
		ASC->GenericGameplayEventCallbacks.FindOrAdd(ReloadDoneTag()).Remove(ReloadEventHandle);
	}
}

void URubezhInventoryComponent::HandleReloadDone(const FGameplayEventData* EventData)
{
	// Способность Lyra в этом же событии посчитает патроны по-своему. Наш расчёт — следующим кадром и окончательно.
	ReloadingWeaponSlot = GetActiveWeaponSlot();
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::FinishReload);
}

void URubezhInventoryComponent::FinishReload()
{
	// За кадр игрок мог сменить оружие или погибнуть: тогда магазин не меняем, только возвращаем верные числа.
	const int32 WeaponSlot = ReloadingWeaponSlot;
	ReloadingWeaponSlot = INDEX_NONE;
	if (!IsOwnerDead() && WeaponSlot == GetActiveWeaponSlot() && WeaponSlots.IsValidIndex(WeaponSlot) && !WeaponSlots[WeaponSlot].ItemId.IsNone())
	{
		if (const FRubezhWeaponDefinition* Weapon = Rubezh::Items::FindWeapon(WeaponSlots[WeaponSlot].ItemId))
		{
			Rubezh::Weapons::Reload(Grid, WeaponSlots[WeaponSlot], *Weapon, [this](FName Id) { return FindDefinition(Id); });
		}
	}
	// В любом случае переписываем копию на предмете Lyra: её расчёт не должен остаться в силе.
	HandleGridChanged();
}

// --- Выстрел ---

int32 URubezhInventoryComponent::FindWeaponSlot(const ULyraInventoryItemInstance& Item) const
{
	return WeaponInstances.IndexOfByPredicate([&Item](const TObjectPtr<ULyraInventoryItemInstance>& Instance) { return Instance == &Item; });
}

const FRubezhWeaponDefinition* URubezhInventoryComponent::FindWeaponByInstance(const ULyraInventoryItemInstance& Item) const
{
	// У клиента экземпляров нет: только что выстрелившее оружие — то, что сейчас в руках.
	const int32 WeaponSlot = GetOwner()->HasAuthority() ? FindWeaponSlot(Item) : GetActiveWeaponSlot();
	return WeaponSlots.IsValidIndex(WeaponSlot) ? Rubezh::Items::FindWeapon(WeaponSlots[WeaponSlot].ItemId) : nullptr;
}

float URubezhInventoryComponent::GetWeaponDamage(const UObject* LyraItem, double DistanceCm)
{
	const ULyraInventoryItemInstance* Item = Cast<ULyraInventoryItemInstance>(LyraItem);
	const int32 Damage = Item ? Item->GetStatTagStackCount(RubezhGameplayTags::Weapon_Damage) : 0;
	if (Damage <= 0)
	{
		return -1.0f;
	}
	const int32 Range = Item->GetStatTagStackCount(RubezhGameplayTags::Weapon_EffectiveRange);
	const float Result = Damage * Rubezh::Weapons::GetDistanceMultiplier(DistanceCm, Range);
	UE_LOG(LogRubezh, Verbose, TEXT("Урон оружия: %d, дальность %d м, расстояние %.1f м → %.1f"), Damage, Range, DistanceCm / 100.0, Result);
	return Result;
}

bool URubezhInventoryComponent::HandleShotCommitted(UObject* LyraItem, APawn* Pawn)
{
	ULyraInventoryItemInstance* Item = Cast<ULyraInventoryItemInstance>(LyraItem);
	URubezhInventoryComponent* Inventory = FindForActor(Pawn);
	if (!Item || !Pawn || !Inventory)
	{
		return true;
	}

	// Решение об осечке приходит от сервера заранее: клиент не показывает выстрел, которого не будет.
	const bool bMisfire = Item->GetStatTagStackCount(RubezhGameplayTags::Weapon_NextShotMisfires) > 0;
	const FRubezhWeaponDefinition* Weapon = Inventory->FindWeaponByInstance(*Item);

	if (Pawn->IsLocallyControlled())
	{
		if (bMisfire)
		{
			if (URubezhInteractionComponent* Interaction = Pawn->FindComponentByClass<URubezhInteractionComponent>())
			{
				Interaction->ShowNotice(TEXT("Осечка"));
			}
		}
		else if (Weapon)
		{
			Inventory->ApplyRecoil(*Weapon, *Pawn);
		}
	}

	if (Inventory->GetOwner()->HasAuthority())
	{
		Inventory->HandleServerShot(*Item);
	}
	return !bMisfire;
}

void URubezhInventoryComponent::HandleServerShot(ULyraInventoryItemInstance& Item)
{
	const int32 WeaponSlot = FindWeaponSlot(Item);
	if (!WeaponSlots.IsValidIndex(WeaponSlot))
	{
		return;
	}

	FRubezhInventoryEntry& Weapon = WeaponSlots[WeaponSlot];
	// Считаем сами, а не читаем копию у Lyra: между её перезарядкой и нашей копия ещё неверна, и патроны бы размножились.
	Weapon.Rounds = FMath::Max(Weapon.Rounds - 1, 0);
	Weapon.Durability = Rubezh::Weapons::ApplyWear(Weapon.Durability);
	RollNextShot(Weapon);
	HandleGridChanged();
}

void URubezhInventoryComponent::RollNextShot(FRubezhInventoryEntry& Weapon) const
{
	Weapon.bNextShotMisfires = FMath::FRand() < Rubezh::Weapons::GetMisfireChance(Weapon.Durability);
}

void URubezhInventoryComponent::ApplyRecoil(const FRubezhWeaponDefinition& Weapon, APawn& Pawn)
{
	APlayerController* Controller = Cast<APlayerController>(Pawn.GetController());
	if (!Controller)
	{
		return;
	}

	const double Now = GetWorld()->GetTimeSeconds();
	ShotsInBurst = (Now - LastShotTime) > Rubezh::Weapons::BurstResetSeconds ? 0 : ShotsInBurst + 1;
	LastShotTime = Now;

	const ACharacter* Character = Cast<ACharacter>(&Pawn);
	const Rubezh::Weapons::FRecoil Recoil = Rubezh::Weapons::ComputeRecoil(Weapon, ShotsInBurst,
		Character && Character->bIsCrouched, FMath::FRandRange(-1.0f, 1.0f));
	Controller->SetControlRotation(Controller->GetControlRotation() + FRotator(Recoil.Pitch, Recoil.Yaw, 0.0f));
}
