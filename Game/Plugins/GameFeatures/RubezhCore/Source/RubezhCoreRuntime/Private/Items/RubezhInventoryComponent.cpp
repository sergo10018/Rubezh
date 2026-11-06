// РУБЕЖ: Земля после разлома

#include "Items/RubezhInventoryComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Character/LyraHealthComponent.h"
#include "Engine/DataTable.h"
#include "Items/RubezhItemCatalog.h"
#include "Items/RubezhLootContainer.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "Items/RubezhWorldItem.h"
#include "Movement/RubezhMovementSpeed.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "RubezhCoreRuntimeModule.h"
#include "RubezhSettings.h"
#include "Settlement/RubezhSafeZone.h"
#include "Settlement/RubezhTrader.h"
#include "Survival/RubezhSurvivalSet.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RubezhInventoryComponent)

namespace
{
	constexpr float WeightSyncIntervalSeconds = 0.25f;
}

URubezhInventoryComponent::URubezhInventoryComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
}

URubezhInventoryComponent* URubezhInventoryComponent::FindForActor(const AActor* Actor)
{
	if (!Actor)
	{
		return nullptr;
	}
	if (const APawn* Pawn = Cast<APawn>(Actor))
	{
		Actor = Pawn->GetPlayerState();
	}
	else if (const AController* Controller = Cast<AController>(Actor))
	{
		Actor = Controller->PlayerState;
	}
	return Actor ? Actor->FindComponentByClass<URubezhInventoryComponent>() : nullptr;
}

void URubezhInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Содержимое чужого рюкзака — тайна: видит только владелец. Тело обыскивают через ARubezhLootContainer.
	DOREPLIFETIME_CONDITION(URubezhInventoryComponent, Grid, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(URubezhInventoryComponent, Equipped, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(URubezhInventoryComponent, WeaponSlots, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(URubezhInventoryComponent, LootContainer, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(URubezhInventoryComponent, LootGrid, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(URubezhInventoryComponent, SearchingContainer, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(URubezhInventoryComponent, SearchFinishTime, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(URubezhInventoryComponent, Money, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(URubezhInventoryComponent, TraderInUse, COND_OwnerOnly);
}

void URubezhInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	const URubezhSettings* Settings = GetDefault<URubezhSettings>();
	ItemTable = Settings->ItemTable.LoadSynchronous();
	if (!ItemTable)
	{
		UE_LOG(LogRubezh, Error, TEXT("Rubezh: не задана таблица предметов (Project Settings → Game → Rubezh → Item Table)"));
	}

	if (GetOwner()->HasAuthority())
	{
		Equipped.SetNum(Rubezh::Equipment::SlotCount);
		Grid = FRubezhInventoryGrid(Settings->PocketSlots);
		WeaponSlots.SetNum(Rubezh::Weapons::SlotCount);
		WeaponInstances.SetNum(Rubezh::Weapons::SlotCount);
		Money = Settings->StartingMoney;
		ApplyStartingEquipment();
		ApplyStartingWeapons();
		BindReloadEvent();
	}

	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
	{
		CarriedWeightChangedHandle = ASC->GetGameplayAttributeValueChangeDelegate(URubezhSurvivalSet::GetCarriedWeightAttribute())
			.AddUObject(this, &ThisClass::OnCarriedWeightChanged);
	}
	if (APlayerState* PlayerState = Cast<APlayerState>(GetOwner()))
	{
		PlayerState->OnPawnSet.AddDynamic(this, &ThisClass::OnPawnSet);
	}
}

void URubezhInventoryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
	{
		ASC->GetGameplayAttributeValueChangeDelegate(URubezhSurvivalSet::GetCarriedWeightAttribute()).Remove(CarriedWeightChangedHandle);
	}
	if (APlayerState* PlayerState = Cast<APlayerState>(GetOwner()))
	{
		PlayerState->OnPawnSet.RemoveDynamic(this, &ThisClass::OnPawnSet);
	}
	if (ULyraHealthComponent* Health = BoundHealth.Get())
	{
		Health->OnDeathStarted.RemoveDynamic(this, &ThisClass::OnOwnerDeathStarted);
	}
	if (GetOwner()->HasAuthority())
	{
		CloseLoot();
		CancelSearch();
		CloseTrader();
		UnbindReloadEvent();
	}
	GetWorld()->GetTimerManager().ClearTimer(WeightSyncTimer);
	GetWorld()->GetTimerManager().ClearTimer(WeaponInstanceTimer);
	GetWorld()->GetTimerManager().ClearTimer(MagazineLoadTimer);

	Super::EndPlay(EndPlayReason);
}

void URubezhInventoryComponent::OnPawnSet(APlayerState* Player, APawn* NewPawn, APawn* OldPawn)
{
	RefreshOwnerWalkSpeed();

	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	// Смерть персонажа — сигнал выложить на землю то, что теряется.
	if (ULyraHealthComponent* OldHealth = BoundHealth.Get())
	{
		OldHealth->OnDeathStarted.RemoveDynamic(this, &ThisClass::OnOwnerDeathStarted);
	}
	BoundHealth = ULyraHealthComponent::FindHealthComponent(NewPawn);
	if (ULyraHealthComponent* NewHealth = BoundHealth.Get())
	{
		NewHealth->OnDeathStarted.AddUniqueDynamic(this, &ThisClass::OnOwnerDeathStarted);
	}

	// Атрибуты веса появляются на ASC чуть позже персонажа (их выдаёт набор способностей героя) —
	// ждём их и выставляем вес и лимит с учётом рюкзака.
	if (NewPawn)
	{
		GetWorld()->GetTimerManager().SetTimer(WeightSyncTimer, this, &ThisClass::TrySyncWeightAttributes, WeightSyncIntervalSeconds, true);
		RebuildWeaponInstances();
	}
}

void URubezhInventoryComponent::TrySyncWeightAttributes()
{
	const UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
	if (ASC && ASC->GetSet<URubezhSurvivalSet>())
	{
		UpdateCarriedWeight();
		GetWorld()->GetTimerManager().ClearTimer(WeightSyncTimer);
	}
}

void URubezhInventoryComponent::OnCarriedWeightChanged(const FOnAttributeChangeData& ChangeData)
{
	RefreshOwnerWalkSpeed();
}

void URubezhInventoryComponent::RefreshOwnerWalkSpeed() const
{
	if (const APlayerState* PlayerState = Cast<APlayerState>(GetOwner()))
	{
		Rubezh::Movement::RefreshWalkSpeed(Cast<ACharacter>(PlayerState->GetPawn()));
	}
}

const FRubezhItemDefinition* URubezhInventoryComponent::FindDefinition(FName ItemId) const
{
	return ItemTable ? ItemTable->FindRow<FRubezhItemDefinition>(ItemId, TEXT("RubezhInventory"), false) : nullptr;
}

int32 URubezhInventoryComponent::GetPocketSlots()
{
	return GetDefault<URubezhSettings>()->PocketSlots;
}

APawn* URubezhInventoryComponent::GetOwnerPawn() const
{
	const APlayerState* PlayerState = Cast<APlayerState>(GetOwner());
	return PlayerState ? PlayerState->GetPawn() : nullptr;
}

void URubezhInventoryComponent::ApplyStartingEquipment()
{
	auto Lookup = [this](FName Id) { return FindDefinition(Id); };
	for (const FName& ItemId : GetDefault<URubezhSettings>()->StartingEquipment)
	{
		const FRubezhItemDefinition* Item = FindDefinition(ItemId);
		const int32 SlotIndex = Item ? Rubezh::Equipment::GetSlotIndex(Item->EquipSlot) : INDEX_NONE;
		if (SlotIndex == INDEX_NONE || !Equipped[SlotIndex].IsNone())
		{
			UE_LOG(LogRubezh, Warning, TEXT("Rubezh: стартовое снаряжение '%s' не надевается или слот занят"), *ItemId.ToString());
			continue;
		}
		Equipped[SlotIndex] = ItemId;
	}
	// Рюкзак пока пустой — просто берём число ячеек надетого рюкзака.
	Grid = FRubezhInventoryGrid(Rubezh::Equipment::GetSlotCount(Equipped, Lookup, GetPocketSlots()));
	HandleGridChanged();
}

FName URubezhInventoryComponent::GetEquipped(ERubezhEquipSlot Slot) const
{
	const int32 Index = Rubezh::Equipment::GetSlotIndex(Slot);
	return Equipped.IsValidIndex(Index) ? Equipped[Index] : NAME_None;
}

Rubezh::Equipment::FProtection URubezhInventoryComponent::GetProtection(ERubezhBodyZone Zone) const
{
	return Rubezh::Equipment::GetProtection(Equipped, Zone, [this](FName Id) { return FindDefinition(Id); });
}

float URubezhInventoryComponent::GetIncomingDamageMultiplier(const UAbilitySystemComponent* Target, const FHitResult* Hit)
{
	// В поселении никого нельзя ранить, даже стреляя в него снаружи.
	const AActor* TargetAvatar = Target ? Target->GetAvatarActor() : nullptr;
	if (TargetAvatar && ARubezhSafeZone::IsInsideAny(TargetAvatar->GetWorld(), TargetAvatar->GetActorLocation()))
	{
		return 0.0f;
	}

	// Попадание в голову опаснее, в конечности — слабее; это верно и для целей без снаряжения.
	const ERubezhBodyZone Zone = Hit ? Rubezh::Equipment::GetZoneForBone(Hit->BoneName) : ERubezhBodyZone::Torso;
	const float ZoneMultiplier = Rubezh::Weapons::GetZoneDamageMultiplier(Zone);
	UE_LOG(LogRubezh, Verbose, TEXT("Попадание: кость %s, %s, множитель %.2f"), Hit ? *Hit->BoneName.ToString() : TEXT("нет"), Rubezh::Equipment::GetZoneName(Zone), ZoneMultiplier);

	const URubezhInventoryComponent* Inventory = Target ? FindForActor(Target->GetOwnerActor()) : nullptr;
	if (!Inventory)
	{
		return ZoneMultiplier;
	}
	return Inventory->GetProtection(Zone).DamageMultiplier * ZoneMultiplier;
}

void URubezhInventoryComponent::LogEquipFailure(Rubezh::Equipment::EResult Result) const
{
	if (Result != Rubezh::Equipment::EResult::Ok)
	{
		UE_LOG(LogRubezh, Verbose, TEXT("%s: снаряжение — %s"), *GetNameSafe(GetOwner()), Rubezh::Equipment::Describe(Result));
	}
}

void URubezhInventoryComponent::ServerEquipEntry_Implementation(int32 EntryIndex)
{
	const Rubezh::Equipment::EResult Result = Rubezh::Equipment::Equip(Grid, Equipped, EntryIndex, [this](FName Id) { return FindDefinition(Id); }, GetPocketSlots());
	LogEquipFailure(Result);
	if (Result == Rubezh::Equipment::EResult::Ok)
	{
		HandleGridChanged();
	}
}

void URubezhInventoryComponent::ServerUnequip_Implementation(ERubezhEquipSlot Slot, int32 TargetSlot)
{
	const TOptional<int32> Target = TargetSlot == INDEX_NONE ? TOptional<int32>() : TOptional<int32>(TargetSlot);
	const Rubezh::Equipment::EResult Result = Rubezh::Equipment::Unequip(Grid, Equipped, Slot, Target, [this](FName Id) { return FindDefinition(Id); }, GetPocketSlots());
	LogEquipFailure(Result);
	if (Result == Rubezh::Equipment::EResult::Ok)
	{
		HandleGridChanged();
	}
}

void URubezhInventoryComponent::ServerDropEquipped_Implementation(ERubezhEquipSlot Slot)
{
	APawn* Pawn = GetOwnerPawn();
	if (!Pawn)
	{
		return;
	}

	// Рюкзак с вещами падает вместе с ними: его можно обыскать или надеть обратно.
	if (Slot == ERubezhEquipSlot::Backpack && !Grid.IsEmpty())
	{
		FRubezhInventoryGrid NewGrid = Grid;
		Rubezh::Equipment::FEquipped NewEquipped = Equipped;
		FName BagId;
		FRubezhInventoryGrid BagContents;
		const Rubezh::Equipment::EResult Result = Rubezh::Equipment::DropBag(NewGrid, NewEquipped, GetPocketSlots(), BagId, BagContents);
		const FRubezhItemDefinition* Bag = FindDefinition(BagId);
		LogEquipFailure(Result);
		if (Result != Rubezh::Equipment::EResult::Ok || !Bag || !ARubezhLootContainer::SpawnDropped(*GetWorld(),
			ARubezhWorldItem::FindDropLocation(*Pawn), Pawn->GetActorRotation().Yaw, Bag->DisplayName, BagContents, BagId, 0.0f))
		{
			return;
		}

		Grid = MoveTemp(NewGrid);
		Equipped = MoveTemp(NewEquipped);
		HandleGridChanged();
		return;
	}

	// Снимаем на копиях; на землю кладём до того, как сохранить, — если положить не вышло, вещь останется надетой.
	FRubezhInventoryGrid NewGrid = Grid;
	Rubezh::Equipment::FEquipped NewEquipped = Equipped;
	FName ItemId;
	const Rubezh::Equipment::EResult Result = Rubezh::Equipment::TakeOff(NewGrid, NewEquipped, Slot, [this](FName Id) { return FindDefinition(Id); }, GetPocketSlots(), ItemId);
	LogEquipFailure(Result);
	if (Result != Rubezh::Equipment::EResult::Ok || !ARubezhWorldItem::SpawnInFrontOf(*Pawn, ItemId, 1))
	{
		return;
	}

	Grid = MoveTemp(NewGrid);
	Equipped = MoveTemp(NewEquipped);
	HandleGridChanged();
}

int32 URubezhInventoryComponent::AddItem(FName ItemId, int32 Count)
{
	if (!GetOwner()->HasAuthority() || Count <= 0)
	{
		return Count;
	}

	const FRubezhItemDefinition* Definition = FindDefinition(ItemId);
	if (!Definition)
	{
		UE_LOG(LogRubezh, Warning, TEXT("Rubezh: неизвестный предмет '%s'"), *ItemId.ToString());
		return Count;
	}

	const int32 NotAdded = Grid.AddItem(ItemId, *Definition, Count);
	if (NotAdded < Count)
	{
		HandleGridChanged();
	}
	return NotAdded;
}

int32 URubezhInventoryComponent::AddEntry(const FRubezhInventoryEntry& Entry)
{
	const FRubezhItemDefinition* Definition = FindDefinition(Entry.ItemId);
	if (!GetOwner()->HasAuthority() || !Definition || Entry.Count <= 0)
	{
		return Entry.Count;
	}

	// Несколько одинаковых штучных предметов (поставили на карту стопкой) — обычное добавление.
	const int32 NotAdded = Entry.Count > 1 ? Grid.AddItem(Entry.ItemId, *Definition, Entry.Count) : Grid.AddEntry(Entry, *Definition);
	if (NotAdded < Entry.Count)
	{
		HandleGridChanged();
	}
	return NotAdded;
}

int32 URubezhInventoryComponent::RemoveItem(FName ItemId, int32 Count)
{
	if (!GetOwner()->HasAuthority() || Count <= 0)
	{
		return 0;
	}

	const int32 Removed = Grid.RemoveItem(ItemId, Count);
	if (Removed > 0)
	{
		HandleGridChanged();
	}
	return Removed;
}

void URubezhInventoryComponent::ServerMoveEntry_Implementation(int32 EntryIndex, int32 NewSlot)
{
	// Рюкзак сам проверяет индекс и номер ячейки — недопустимый запрос просто ничего не меняет.
	if (Grid.MoveEntry(EntryIndex, NewSlot, [this](FName Id) { return FindDefinition(Id); }))
	{
		HandleGridChanged();
	}
}

void URubezhInventoryComponent::ServerDropEntry_Implementation(int32 EntryIndex)
{
	APawn* Pawn = GetOwnerPawn();
	if (!Pawn || !Grid.GetEntries().IsValidIndex(EntryIndex))
	{
		return;
	}

	// Сначала кладём на землю, потом убираем из рюкзака — если положить не вышло, вещь не пропадёт.
	const FRubezhInventoryEntry Entry = Grid.GetEntries()[EntryIndex];
	if (!ARubezhWorldItem::SpawnEntryInFrontOf(*Pawn, Entry))
	{
		return;
	}

	FRubezhInventoryEntry Removed;
	Grid.RemoveEntry(EntryIndex, Removed);
	HandleGridChanged();
}

float URubezhInventoryComponent::GetTotalWeight() const
{
	auto Lookup = [this](FName Id) { return FindDefinition(Id); };
	float Weight = Grid.GetTotalWeight(Lookup) + Rubezh::Equipment::GetWeight(Equipped, Lookup);
	for (const FRubezhInventoryEntry& Weapon : WeaponSlots)
	{
		const FRubezhItemDefinition* Definition = FindDefinition(Weapon.ItemId);
		const FRubezhWeaponDefinition* Stats = Definition ? Rubezh::Items::FindWeapon(Weapon.ItemId) : nullptr;
		if (Definition && Stats)
		{
			const FRubezhItemDefinition* Ammo = FindDefinition(Stats->AmmoItem);
			const FRubezhItemDefinition* Magazine = (Weapon.bHasMagazine && !Stats->MagazineItem.IsNone()) ? FindDefinition(Stats->MagazineItem) : nullptr;
			Weight += Definition->WeightKg + (Ammo ? Ammo->WeightKg * Weapon.Rounds : 0.0f) + (Magazine ? Magazine->WeightKg : 0.0f);
		}
	}
	return Weight;
}

void URubezhInventoryComponent::OnRep_Grid()
{
	OnInventoryChanged.Broadcast();
}

void URubezhInventoryComponent::OnOwnerDeathStarted(AActor* OwningActor)
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	CloseLoot();
	CancelSearch();
	ServerStopLoadingMagazine_Implementation();
	ClearWeaponInstances();
	if (const APawn* Pawn = Cast<APawn>(OwningActor))
	{
		DropBodyOnDeath(*Pawn);
	}
}

void URubezhInventoryComponent::HandleGridChanged()
{
	UpdateCarriedWeight();
	SyncWeaponTags();
	OnInventoryChanged.Broadcast();
}

void URubezhInventoryComponent::UpdateCarriedWeight() const
{
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
	if (ASC && ASC->GetSet<URubezhSurvivalSet>())
	{
		const float CarryBonus = Rubezh::Equipment::GetCarryBonusKg(Equipped, [this](FName Id) { return FindDefinition(Id); });
		ASC->SetNumericAttributeBase(URubezhSurvivalSet::GetMaxCarryWeightAttribute(), Rubezh::Encumbrance::BaseMaxCarryWeight + CarryBonus);
		ASC->SetNumericAttributeBase(URubezhSurvivalSet::GetCarriedWeightAttribute(), GetTotalWeight());
	}
}
